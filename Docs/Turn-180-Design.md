# 移动中的 180° 快速转身

> 目标：跑动中反向（含连续快速反向）不滑步、不抽搐、不"冰上打转"。
> 前提：`UACTCharacterMovementComponent` 是旋转的唯一权威，动画层只读 `LocomotionState`。

---

## 0. 一句话结论

**连续旋转不要状态，离散 pivot 才要状态。**

- 小角度和中角度转向：靠**旋转提速 + 速度折扣**解决，纯连续量，无状态、永不抖。
- 180° 反打：靠**带 root motion 的蒙太奇**（`PC_01_Jog_Pivot_*_180`）解决。
- "不断快速转身"这个场景，**绝不能**用"角度超过阈值就播 clip"来判定 —— 那会造成蒙太奇连播。
  判据必须是**输入方向是否已经稳定**，而不是**角度有多大**。

---

## 1. 为什么原来的实现转不了 180°

### 1.1 Orientation Warping 主动拒绝 90° 以上（这是设计如此）

`FAnimNode_OrientationWarping::LocomotionAngleDeltaThreshold` 默认 90°，头文件注释原文：

> Specifies an angle threshold to prevent erroneous over-rotation of the character, disabled with a value of 0.
> When the effective orientation warping angle is detected to be greater than this value (default: 90 degrees)
> the locomotion direction will be inverted prior to warping.
> **Example: Playing a forward running animation while the motion is going backward.
> Rather than orientation warping by 180 degrees, the system will warp by 0 degrees.**

也就是说：`|YawOffset| > 90°` 时节点**什么都不做**（把方向取反后 warp 出来是 0°），而不是"warp 90°"。

这是刻意的 —— 把 180° 分摊到脊椎骨上会扭断腰。所以 `YawOffset` 一旦超过 90°，
就进入了一个**动画完全不管**的区间，只能由胶囊旋转和专用 clip 承担。

### 1.2 基础旋转速度不够

`VelocityDirectionInterpSpeed = 540 deg/s` → 转 180° 要 **0.33 秒**。
这段时间里 `YawOffset` 一直贴着 180°，Warping 已放弃，身体硬转 —— 就是"舍不得转身"的观感。

### 1.3 目标角取自速度方向 → 慢半拍

原实现：

```cpp
if (Velocity.SizeSquared2D() > FMath::Square(MinSpeedToOrient))
    TargetYawAngle = Velocity.ToOrientationRotator().Yaw;
```

玩家把摇杆从后推到前，**意图立刻变了，速度要 0.6 秒才掉头**
（`TimeToStop 0.3` 刹停 + `TimeToMaxSpeed 0.35` 反向加速）。
用速度做目标，角色必须"等速度转过来"才开始转身，而且速度穿 0 的那一段方向纯是数值噪声。

### 1.4 状态机播不了带旋转的 clip

`UAnimInstance::RootMotionMode` 默认是 `RootMotionFromMontagesOnly`
（`AnimInstance.cpp:202`）。

pivot clip 的 180° 旋转在 root bone 里。用状态机播 → root motion 被忽略 → 身体在 clip 里转了 180°、
胶囊没转 → 人和朝向对不上。

**所以 pivot 只能走蒙太奇。** 这与现有的 Roll 是同一套机制，不是新引入的复杂度。

---

## 2. 分档设计

| `|TurnAngle|` | 机制 | 资产 | 速度 |
|---|---|---|---|
| `< 45°` | 基础旋转（540 deg/s） + Orientation Warping 全权负责 | 无（Cycle 循环） | 不变 |
| `45° ~ 90°` | 旋转提速到 1080 + Warping 承担前 90° | 无 | 开始折扣 |
| `90° ~ 140°` | 旋转提速到 1080 + 速度折扣到低谷 | 无 | 折扣到 `TurnBrakeMinScale` |
| `> 140°` 且 `Speed` 在 `350~650` | **pivot 蒙太奇**（自带 180° root yaw + 全套位移） | `PC_01_Jog_Pivot_L_180_Lfoot` / `_R_180_Rfoot` | 交给 clip，不折扣 |
| `> 140°` 其它速度 | 只靠旋转提速（0.17 秒），可选接原地转身 clip | `A_PC_01_Turn_180_Left/Right` | 折扣到底 |

分档是**连续的**（角度、速度都是滑块），只有 pivot 这一段是离散事件。
这个边界划得越窄，需要处理的状态就越少。

---

## 3. "不断快速转身"：为什么不能靠 clip

### 3.1 错误做法

```
if (|YawOffset| > 150) 播 pivot
```

玩家绕圈搓摇杆时，`|YawOffset|` 会反复穿过 150° → 每 0.5 秒播一次蒙太奇 →
蒙太奇互相打断 → 角色抽在原地。这是这个需求最容易踩的坑。

### 3.2 正确判据：看输入方向是否**稳定**，不看角度**大小**

两种行为在信号上完全不同：

| 行为 | 输入方向角速度 | 角度 |
|---|---|---|
| 摇杆绕圈 / 左右搓 | **持续偏高**（300~1000 deg/s） | 一直大 |
| 真的反打 | **跳一下，然后立刻归零** | 大且稳定 |

所以 pivot 的触发条件是「**角度大 且 输入方向已经安静了 `TurnStableDwell` 秒**」。
绕圈时角速度永远达不到"安静"，pivot 一次都不会触发，转身完全由连续旋转完成 ——
**无状态、天然抗抖动**。

### 3.3 再加一层冷却

即使判据正确，玩家仍可能以 3~4Hz 的频率"打一下停一下"。所以再叠一个
`PivotCooldown`（默认 0.9s，必须 ≥ 蒙太奇时长）：

- 冷却期内不响应任何 pivot 请求
- 冷却期内的反向由连续旋转承担（0.17s 转 180°，够快，只是没有支撑脚）

结果是：**连续快速转身最多每 0.9 秒一次 pivot，其余都退化为快速旋转。**
这正是想要的降级行为 —— 高频操作时牺牲一点表现力，换取绝对不抽搐。

---

## 4. 实现（已在移动组件里）

### 4.1 目标方向改用输入意图

```cpp
const FVector InputVector = CharacterOwner->GetLastMovementInputVector();
if (InputVector.SizeSquared2D() > 0.01f)
    TargetYawAngle = InputVector.ToOrientationRotator().Yaw;      // 有输入 → 用意图
else if (Velocity.SizeSquared2D() > Square(MinSpeedToOrient))
    TargetYawAngle = Velocity.ToOrientationRotator().Yaw;         // 滑行 → 用速度
// 都没有 → 保持
```

`GetLastMovementInputVector()` 是上一帧被 `ConsumeMovementInputVector` 消费掉的输入（世界空间），
引擎注释明确写了"动画更新应该用它"，且在 `TickComponent` 前后读都有效。
用它替代速度方向的收益：**意图一变，转身立刻开始，不用等速度掉头**。

### 4.2 pivot 的四个门

```cpp
if (PivotCooldownTimer <= 0
    && AbsOffset > PivotAngleThreshold      // 1. 角度够大
    && Speed > PivotMinSpeed                // 2. 速度在窗口内（见 6.5 ②）
    && Speed < PivotMaxSpeed
    && TurnStableTimer >= TurnStableDwell   // 3. 输入方向已稳定
    && IsMovingOnGround())                  // 4. 在地上
{
    PivotCooldownTimer = PivotCooldown;
    OnPivotRequested.Broadcast(TurnAngleNow);   // 事件，不是 bool
}
```

用**事件**而不是每帧可查的 bool：pivot 是"触发一次、播一次"的离散行为，
bool 会被状态机重复读到 `true` 而连播。

### 4.3 旋转提速

```cpp
if (RotationInterpSpeedOverride < 0.0f)   // 攻击锁朝向时不被顶掉
{
    const float SpeedBlend = Clamp((AbsOffset - SharpTurnAngleThreshold)
                                   / (180 - SharpTurnAngleThreshold), 0, 1);
    InterpSpeed = Lerp(InterpSpeed, SharpTurnInterpSpeed, SpeedBlend);
}
```

180° → 1080 deg/s → 0.17 秒。用 `Lerp` 而不是硬切，避免转到一半突然加速。

### 4.4 顺带修掉一个隐患：root motion 后的朝向回弹

`PhysicsRotation` 缓存了 `CurrentYawAngle`。蒙太奇 root motion 转了朝向之后，
缓存还是旧值 → 下一帧 `FixedTurn` 从旧角度出发 → **角色被拽回去**（表现为翻滚结束"回弹一下"）。

```cpp
if (HasAnimRootMotion())
{
    CurrentYawAngle = CharacterOwner->GetActorRotation().Yaw;  // 跟着走
    return;
}
// 传送 / SetActorRotation 同理，阈值 1° 滤掉自身的浮点误差
if (Abs(NormalizeAxis(ActorYaw - CurrentYawAngle)) > 1.0f)
    CurrentYawAngle = ActorYaw;
```

---

## 5. 速度包络：它和 root motion 是**互斥**的两条路，不是叠加

```cpp
BrakeAlpha = Clamp((AbsOffset - TurnBrakeStartAngle) / (180 - TurnBrakeStartAngle), 0, 1);
TargetScale = Lerp(1.0f, TurnBrakeMinScale, BrakeAlpha);        // 平滑后
MaxWalkSpeed = SmoothedMaxSpeed * TurnSpeedScale;

// 两条路必须互斥，否则互相打架
bPivotOwnsMotion = HasAnimRootMotion() || (bPivotCandidate && TurnStableTimer > 0);
TargetScale = bPivotOwnsMotion ? 1.0f : TargetScale;
```

### 5.1 会不会互相影响：会，而且原来就有一处真的在打架

RM 期间 `Velocity` 被整个替换，折扣**无效但不至于有害**。真正的问题是**切换的那一帧**：

- `TurnStableDwell = 0.1s`，这 0.1 秒里 `|TurnAngle| = 180°` → 折扣已经开始压速度
- `MaxWalkSpeed` 从 500 掉到约 265，实际速度被刹到 **330 左右**
- 蒙太奇开始，clip 的 lead-in 段 RM 速度是 **500**，`ConstrainAnimRootMotionVelocity` 把 `Velocity` 一帧拉回 500

→ **进入蒙太奇瞬间速度跳 330 → 500，约 50% 的突变**，肉眼可见。

修法就是上面的 `bPivotOwnsMotion`：把「要不要播 pivot」的谓词（`bPivotCandidate`）复用给折扣判断，
只要输入已经安静（`TurnStableTimer > 0`，说明这一帧起就只是在等 dwell 走完），折扣立刻让路。

反过来这个写法还有个好处：**绕圈时输入永远不安静 → `TurnStableTimer` 恒为 0 → 折扣全量生效**，
正是想要的行为。一个谓词同时解决了两个方向的判定。

### 5.2 更该知道的：包络对"单次 180°"其实来不及

按 Run = 500 把时间账算一遍：

| 动作 | 耗时 | 依据 |
|---|---|---|
| 胶囊转完 180° | **0.17s** | `SharpTurnInterpSpeed = 1080 deg/s` |
| 折扣插值到接近目标 | **~0.3s** | `FInterpTo`，`TurnSpeedSmoothingRate = 8/s` |
| 实际速度被刹下去 | **~0.26s** | `BrakingDecelerationWalking = 500 / TimeToStop(0.3)` |

**折扣还没起作用，旋转已经结束了。** 单次快转的滑步，包络只能削掉约 1/4（85cm → 65cm），救不回来。

这不是参数问题，是时序问题：想更快只能提高 `BrakingDecelerationWalking`，而那是全步态共用的，
会把正常刹车也变硬。

### 5.3 所以两者的定位要分清

| | 归属 | 管用范围 |
|---|---|---|
| pivot 蒙太奇 | 单次大角度反打（偶发、离散） | 靠 clip 的动作本身消除滑步，与包络无关 |
| 速度包络 | **持续转向**（绕圈、连续调整） | 角度长期很大 → 折扣稳定维持 → 滑步量才真的降下来 |

一句话：**clip 管"那一下"，包络管"一直在转"。** 它们的时间尺度差一个数量级，
所以互斥反而比叠加更清晰 —— 让每个机制只在自己擅长的区间里工作。

正因为包络最终只服务于"持续转向"，`TurnBrakeMinScale`（默认 0.15）是最该先调的一个参数：
绕圈时它会稳定在 75 cm/s，觉得像慢走就提到 0.3~0.4。

### 5.4 一个实现细节

`TurnSpeedScale` 只乘在 `MaxWalkSpeed` 上，**不回写 `SmoothedMaxSpeed`** ——
否则折扣会被步态插值再平滑一次，转身结束后速度恢复会拖一条长尾。

调参：
- 绕圈**太慢** → 提高 `TurnBrakeMinScale`
- 绕圈**还在滑** → 降低 `TurnBrakeMinScale`，或把 `TurnBrakeStartAngle` 前移到 40
- 单次反打**进蒙太奇时速度跳** → 检查 `PivotMinSpeed/MaxSpeed` 窗口是否匹配 clip 的设计速度

---

## 6. ABP 接线

### 6.1 加一个 `DefaultSlot`（全身体蒙太奇专用）

现在 `ABP-v1-Wiring.md` 里只有一个 `Slot "UpperBody"`，但 pivot / roll 是**全身**动作，
且蒙太奇默认写的是 `DefaultSlot`。拓扑改成：

```
Root
└─ Foot Placement
   └─ Slot "UpperBody"        ← Phase 6 攻击，用 LayeredBlendPerBone 只覆盖 spine 以上
      └─ Slot "DefaultSlot"  ← 全身：roll / pivot / mantle
         └─ Stride Warping
            └─ Orientation Warping
               └─ State Machine
```

- `DefaultSlot` 放在 Warping **之后**：pivot 是手工 K 的动画，不该被 Stride/Orientation 再扭一次。
- `Foot Placement` 仍在**最后**：pivot 的支撑脚也要贴地，这是它比 Warping 更该在后面的原因。
- `UpperBody` 在 `DefaultSlot` 之后：将来攻击只覆盖上半身时，下半身还能跑。

### 6.2 pivot 期间状态机不用改

蒙太奇走 Slot，不经过状态机。底下的 `Cycle` 照常在跑（`Speed` 仍然 > 0），
蒙太奇播完自然淡出。**不要**为 pivot 加一个状态机的状态 —— 那会把"离散事件"变成"离散状态"，
而状态需要进出条件，正是抖动来源。

### 6.3 蒙太奇期间不要进 Start / Stop

pivot 期间速度被压到 75，`Cycle → Stop` 的条件是 `MoveInput < 0.2 && Speed > 20`。
玩家反打时输入是满的，不会误触发。但如果做"反向输入 + 松开"的复合操作，可能误进 Stop。
保险做法：`Cycle → Stop` 再加一条 `!IsAnyMontagePlaying()`（ABP 里用 `Slot` 的 `On Update`
或者 `GetAnimInstance()->IsAnyMontagePlaying()` 节点判断）。

### 6.4 蒙太奇"播完之后"到底发生了什么

关键认知：**状态机从来没有离开过。** pivot 不是状态机的一个状态，它播在 `Slot` 节点上：

```
State Machine（Cycle，一直在跑，条件每帧都在评估）
        │
        ▼
  Slot "DefaultSlot"   ── 蒙太奇权重 1 时完全覆盖状态机输出
        │
        ▼
  BlendOut 期间权重 1 → 0，状态机姿势淡回来
```

所以"回到其他状态"不是一次切换，而是 **Slot 权重淡出**。分三层看：

| 层 | 什么时刻发生 | 由谁控制 |
|---|---|---|
| 逻辑层 | `Montage->GetPlayLength()` 播完 | `bEnableAutoBlendOut`（默认 true） |
| 表现层 | 之后 0.25s，Slot 权重 1→0 | 蒙太奇的 `BlendOut` |
| 位置层 | 同一刻：`HasAnimRootMotion()` 变 false，`Velocity` 交还 `CalcVelocity` | 引擎 |

**三层不同步是"回位生硬"的根源**：BlendOut 那 0.25 秒里，位置已经不由 clip 决定了，
但姿势还在收尾。这次不需要额外处理 —— `PC_01_Jog_Pivot` 的最后一段就是"已经转完身正在跑"，
和底层 `Cycle` 的姿势本来就接近，0.25s 淡出看不出来。换成"撑地转身后急停"的 clip 就必须改。

**底下的状态机靠 `LocomotionState.Speed` 判断走向，而蒙太奇期间 `Velocity` 被 RM 速度替换，
所以 `Speed` 是 clip 的真实移动速度** —— `Cycle` 不会因为"玩家没在按键盘"误退出。
这是 RM 接管 Velocity 的副产品，不是巧合。

位置层交还时会有一次轻微的速度对齐：RM 末速度（≈500）与 `MaxWalkSpeed`（≈500）接近，
`IsExceedingMaxSpeed` 最多做一次轻微刹车，看不出来。**前提是 `TurnSpeedScale` 在出蒙太奇时已经回到 1**
—— 这也是 4.4 里"`HasAnimRootMotion()` 时强制折扣为 1"的原因。

唯一要注意的坑：pivot 的转身瞬间 clip 速度会短暂掉到很低。如果那一帧 `MoveInput` 也恰好 < 0.2，
`Cycle → Stop` 会误触发（Stop 在蒙太奇底下开始播，淡出时露出来）。这就是 6.3 那条
`!IsAnyMontagePlaying()` 的实际用途，不是保险，是必需的。

### 6.5 这个 clip 是"三段式"，带来三个必须处理的后果

`当前移动一段距离 → 转身 → 转身后继续移动一段距离`，意味着：

**① 蒙太奇期间位移完全由 clip 决定，玩家没有控制权。**

`CharacterMovementComponent.cpp:2952-2969`：

```cpp
if (CurrentRootMotion.HasOverrideVelocity() || HasAnimRootMotion())
{
    if (HasAnimRootMotion())
    {
        AnimRootMotionVelocity = CalcAnimRootMotionVelocity(...);
        Velocity = ConstrainAnimRootMotionVelocity(AnimRootMotionVelocity, Velocity);  // 整个替换
    }
}
```

是**替换**，不是叠加。所以：

- 不会双重位移（之前担心的 RM + velocity 双倍移动，实际不存在）
- `MaxWalkSpeed` / `TurnSpeedScale` 在蒙太奇期间**完全无效**
- 不需要手动清零速度，引擎已经做了
- `PhysicsRotation` 被跳过（`bAllowPhysicsRotationDuringAnimRootMotion = false` 已设）

**② 进入速度必须匹配 clip 的设计速度。**

clip 是按某个速度 K 的。进入速度 300 而 K = 500，蒙太奇第一帧 RM 就把速度硬拉到 500 ——
肉眼可见的速度突变。所以 `PivotMinSpeed` / `PivotMaxSpeed` 是**窗口**（350~650），不是下限。

Sprint(800) 没有对应的 sprint pivot 资产，会被上限挡掉，退回纯旋转路径 —— 这是正确的降级，
不是缺陷。要支持冲刺转身，得先找到/制作 sprint 版 pivot clip。

**③ 贴墙 pivot 不会穿墙，但会"白播"（修正之前"会穿墙"的说法）。**

位移走的是 `PhysWalking → SafeMoveUpdatedComponent(Adjusted, Quat, true, Hit)`
（`CharacterMovementComponent.cpp:4456`），`bSweep = true`，碰撞完全生效。
**不会穿墙。** 真正的失败模式是下面三条，更隐蔽：

**(a) 旋转无条件成功，位移被挡 → 动画与位置脱节。**

旋转是在位移之后**单独**应用的（`:3053-3060`）：

```cpp
// change position
StartNewPhysics(DeltaSeconds, 0);          // ← 位移，带扫掠，会被挡

// Apply Root Motion rotation after movement is complete.
if (HasAnimRootMotion())
{
    MoveUpdatedComponent(FVector::ZeroVector, NewActorRotationQuat, true);   // ← delta 为零
}
```

delta 为零，而胶囊绕 Z 轴对称、**旋转不改变占用空间**，所以这一句永远不会被墙挡住。
结果：转身那一刻照常转过去，但"接近段"和"离开段"的位移全部丢失了 ——
脚踩的位置和实际地面完全不符，滑步比不播 clip 还严重。

**(b) 不是停住，而是沿墙滑。**

`Hit.Time < 1` 之后走 `SlideAlongSurface(Adjusted, 1 - Hit.Time, Hit.Normal, ...)`（`:4483`），
角色会沿墙的切向滑一段。位置不可预测，结束点可能离 clip 设计的位置很远。

**(c) 最坑的一条：RM 期间 `Velocity` 不由实际位移反推。**

```cpp
if( !bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
{
    Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
}
```

条件里带 `!HasAnimRootMotion()`。所以被墙挡到实际速度为 0 时，`Velocity` 仍然是
**clip 的设计速度**（500）。于是 `LocomotionState.Speed` 报 500 —— Blend Space 采样、
`Cycle → Stop` 判断、Lean 全部基于一个假速度。出蒙太奇后 `CalcVelocity` 又从这假速度
开始算，角色会顶着墙"跑步"。

**所以通路检查仍然值得做，但目的改了**：不是"防穿墙"（引擎已经防了），
而是"避免播一段位置对不上的动画"。收益从"修 bug"降级为"避免难看"，
优先级可以放低 —— 先跑起来看实际表现再决定要不要加。

真要加的话，有两个成本不同的方案：

1. **触发前扫掠**（推荐，一次性开销）：从 clip 的 `Root Motion` 面板读出转身前/后的两段位移长度，
   触发前用胶囊沿 `当前朝向` 和 `新朝向` 各扫一次（`SweepSingleByChannel`）。任一被挡就不播。
2. **运行中检测**：pivot 期间每帧比较"实际位移 / (Velocity·dt)"，连续 0.15 秒低于 30%
   就提前 `StopMontages`。不需要预先测量，但要给移动组件加一条"取消动作"的出口。

### 6.6 `OnPivotRequested` 绑在哪里：角色侧，不是 AnimBP

**当前 C++ 已经绑好了**（`AACTCharacterBase::PostInitializeComponents` 里
`AddDynamic(this, &HandlePivotRequested)`）。蓝图子类**不需要再绑**，只要填
`PivotMontageLeft` / `PivotMontageRight` 两个资产。

> ⚠️ 如果你在角色蓝图里又加一条 `Bind Event to On Pivot Requested`，
> **同一帧会播两次**（C++ 一条、蓝图一条）。要改到蓝图里绑，必须先把 C++ 的 `AddDynamic` 删掉。

技术上 AnimBP 也能绑（`TryGetPawnOwner` → `Get Movement Component` → 绑），
但**不该**，四条理由：

1. **和 Roll 保持一条路。** Roll 已经在角色类里 `PlayAnimMontage`，pivot 是同一类行为。
   Phase 6 接 `ActionLogicComponent` 时只改一处；反过来先在 ABP 里绑，将来要从 ABP 往外拆，
   方向是反的。

2. **避免出现第二条蒙太奇入口。** 动作系统（谁在播、能不能被打断、优先级）需要知道所有
   播蒙太奇的请求。ABP 私自播 = 动作系统不知道，之后必然出现"pivot 还在播时攻击插进来"这类打架。

3. **AnimInstance 会重建。** 换装、`SetSkeletalMesh`、`SetAnimInstanceClass` 都会重建 AnimInstance。
   动态委托里存的是**弱引用**（`TScriptDelegate::Object` 的类型是 `FWeakObjectPtr`），
   所以旧实例被回收后 `Broadcast` 会直接跳过，**不会崩也不会阻止 GC**。
   但那个条目会留在多播列表里，每次重建多一条死条目，除非显式 `RemoveAll`。
   放在角色上不存在这个问题 —— 绑定者的生命周期和组件一致，`AddDynamic` 只做一次。

4. **线程。** 绑定的委托事件在**广播方**的线程执行 —— `Broadcast` 发生在 CMC 的 tick（GT），
   所以今天绑在 ABP 里也是 GT，安全。但一旦有人把这段逻辑挪进
   `BlueprintThreadSafeUpdateAnimation`（很容易"顺手"挪），就变成 WT 里调 `Montage_Play`，
   属于未定义行为。角色侧没有这个陷阱。

**什么情况下反而应该放 ABP**：纯表现、不影响位移、不需要被动作系统查询或打断，
并且多个骨架相同的角色共用一个 ABP、想少配一遍资产。pivot 恰好相反 —— 它接管位移和旋转，
必须服从动作优先级，所以不在这类里。

### 6.7 蓝图侧控制：从哪一秒开始、在哪一秒结束

`HandlePivotRequested` 已改成 **`BlueprintNativeEvent`**，C++ 默认实现保留（播 L/R 蒙太奇），
蓝图子类可以整个重写。配套三个蓝图接口：

| 接口 | 用途 |
|---|---|
| `PlayMontageFromTime(Montage, StartTime, PlayRate)` | 从指定秒数开始播。返回"还剩多久"，可直接拿去设定时器 |
| `StopPivotMontage(BlendOutTime)` | 提前结束 pivot（只停 pivot，不误停翻滚/攻击） |
| `GetPivotMontagePosition()` | 当前播放位置（秒）。没在播返回 0 |

#### 用 Section，不要用裸时间

蒙太奇编辑器里把 clip 切成三段，`Play Anim Montage` 节点就能直接选 `StartSectionName`：

```
| LeadIn（接近，跑向回转点） | Turn（撑地 + 转身 180°） | Depart（转身后继续跑） |
```

裸时间的问题是**脆**：clip 重新导入、补帧、改长度都会让时间偏移，而你写死的数字不会变。
Section 跟着资产走，重导也不会错位。所以：

- **跳过 lead-in** → 从 `Turn` 段开始播（用 `Play Anim Montage` 的 `StartSectionName` 即可，不必用 `PlayMontageFromTime`）
- **停在转身完成处** → 见下

`PlayMontageFromTime` 只在"必须按绝对时间定位"时才用（例如程序化算出的插值点）。

#### 及时结束：用 AnimNotify，不要轮询

在 `Turn` 段的最后一帧放一个 `AnimNotify`，蓝图里接它 → `StopPivotMontage(0.15)`。

比另外两种做法都稳：
- 轮询 `GetPivotMontagePosition() > X`：要自己算 `X`，而且 clip 一改就错
- 按 `Montage_Play` 返回的时长设 `Set Timer`：中途被 blend 或改速率就失准

#### 停止点必须在旋转完成之后

`Montage_Stop` 会**同时截断 root motion**。所以：

| 停在哪 | 结果 |
|---|---|
| 旋转**未完成**就停 | 剩余角度由 `PhysicsRotation` 以 1080deg/s 补完 → 看到一段"硬转" |
| 旋转**刚完成**就停 | 干净。`YawOffset ≈ 0`，`PhysicsRotation` 什么都不做 |
| 播完 `Depart` 再停 | 也能用，但多占 0.3~0.5 秒控制权 |

停在 `Turn` 段末尾是甜点：转身是动画做的，离开段交还玩家。

#### 停止之后引擎侧会自动收敛，不用额外处理

- `HasAnimRootMotion()` 变 false → `Velocity` 交还 `CalcVelocity`，此时 `Velocity` 是 clip 在该帧的
  RM 速度（已经是新方向），所以不会有速度跳变
- `CurrentYawAngle` 由 4.4 的漂移保护自动对齐 actor yaw → 不会回弹
- `TurnSpeedScale` 因为 `|YawOffset| ≈ 0` 立刻回到 1 → 不会拖减速尾巴
- `PhysicsRotation` 重新接管 → 残余小角度继续被修正

唯一要留意：`BlendOut` 期间 `HasAnimRootMotion()` 仍是 true，RM 还在应用。
所以淡出时间（0.15~0.2s）内的位移会继续 —— 这是好事，免得急停。

### 6.9 怎么拆 Section：不用切资产

**结论：不需要切动画资源。** Section 是 `UAnimMontage::CompositeSections` 里的**时间标记数组**，
底层仍然引用同一份 `AnimSequence`。切资产不会多出任何能力，只会多出维护成本。

#### 前置：这些 pivot clip 现在是 AnimSequence，还不是 Montage

`PC_01_Jog_Pivot_L_180_Lfoot` 的前缀是 `PC_01_`（序列），不是 `AM_`（蒙太奇）。
所以要先建蒙太奇：

> 右键 Sequence → **Create** → **Create AnimMontage** → 命名 `AM_PC_01_Jog_Pivot_L_180`

左右各建一个，再填进 `PivotMontageLeft` / `PivotMontageRight`。

#### 三步操作

1. 打开 Montage，时间轴底部找到 **Montage Section** 轨道（和 Notify 轨道并列）
2. 在**回转点位置**右键 → **New Montage Section** → 命名 `Turn`
   （菜单项是右键在时间轴的**具体时间点**上触发的，section 就建在你点的位置）
3. 右键该 section → **Edit Section Start Time** 精确输入秒数（拖动会有误差）

加完 `Turn` 之后，原来的 `Default` section 会自动变成只覆盖 `[0, TurnStart)`
—— 把它重命名成 `LeadIn`。再在转身结束处加一个 `Depart`，三段就齐了：

| Section | 区间 | 内容 |
|---|---|---|
| `LeadIn` | `[0, 回转点)` | 跑向回转点 |
| `Turn` | `[回转点, 转身完成)` | 撑地 + 转 180° |
| `Depart` | `[转身完成, 结尾)` | 转身后继续跑 |

每个 section 的边界都是 float 秒，**没有帧对齐限制**，可以精确到任意精度。

#### 怎么找到"回转点"的准确时间

不要靠肉眼拖。三个更可靠的办法：

1. 打开 Sequence 的 **Root Motion** 面板看位移曲线，找**速度最低点**（撑地那一刻）
2. 在 Sequence 里加一个临时 AnimNotify 标出支撑脚落地帧，Montage 里会显示这个 notch，对着它拖
3. 播放时盯支撑脚：脚掌完全贴地、身体开始扭转的那一帧

#### 从 `Turn` 开始播的代价（重要）

clip 在撑地瞬间的 RM 速度接近 0，而玩家进来时可能还在 500。
蒙太奇一开始 `Velocity` 就被 RM 速度替换 → **一帧之内掉到接近 0**。

- 观感上是"急停 + 转身"，对 180 反打来说本来就是这个意思
- 觉得太硬，就把 `Turn` 起点往前挪一点到"速度刚开始下降"处，留 0.1 秒缓冲
- 另一个副作用：`LeadIn` 的位移不会被应用，所以角色是**原地撑地转身**，而不是"跑过去再转"

#### 反过来：保留 `LeadIn` 的取舍

| | 从 `LeadIn` 播 | 从 `Turn` 播 |
|---|---|---|
| 速度 | 连续 | 一帧掉到撑地速度 |
| 位移 | 完整（跑过去 → 转 → 跑回来） | 只保留转身后的位移 |
| 响应 | 延迟 = lead-in 时长 | 立刻转 |

**建议先用完整播放（`StartSection = None`）测一次手感**，再决定要不要跳。
如果 lead-in 只有 0.15~0.2 秒，这个延迟可能反而更自然。

#### 蓝图里的最终形态

重写 `HandlePivotRequested`，整个方法只要两个节点：

```
PlayPivotFromSection(TurnAngle, "Turn")     → 播，并记录 ActivePivotMontage
[`Turn` 段末尾的 AnimNotify] → StopPivotMontage(0.15)
```

`PlayPivotFromSection` 一次做完四件事：按 TurnAngle 选左右、`Montage_Play`（`bStopAllMontages = false`）、
跳到 Section、记录 `ActivePivotMontage`。返回值是**蒙太奇总时长**（已按 PlayRate 折算），
不是"从 Section 起还剩多久"—— 要剩余时长用 `GetPivotMontagePosition()` 自己算。

#### 什么时候才真的该切资产

只有这三种情况：

- `Depart` 段要交给 `Sequence Player` **循环**播（例如"转身后接持续跑"）
- 某一段要喂给 Blend Space 采样
- 要做 Motion Matching 的 clip 库

切分的代价：**RM 连续性断开**（切出来的第二段，根骨起点被当作原点，前段位移丢失
—— 效果和从 section 中间开始播完全一样，所以切分并不会带来额外能力）、
改原始 clip 要同步三份、section 边界在 sequence 里没有标记容易漂移。

### 6.8 为什么"改状态机实现"是更差的选择，不是更好的退路

用户的备选方案是"如果蓝图侧做不到就改状态机"。实际上蓝图侧做得到（见 6.7），
但更重要的是：**状态机这条路本身有硬限制**。

`UAnimInstance::RootMotionMode` 默认 `RootMotionFromMontagesOnly`（`AnimInstance.cpp:202`），
所以**状态机里播的 clip，它的 root motion 不会驱动胶囊**。要想让状态机吃到 RM，只能改成
`RootMotionFromEverything` —— 那会让**所有**状态（包括循环的 `Cycle` Blend Space）的 RM 都开始驱动胶囊。
如果 locomotion 那几个循环 clip 带 RM，角色会自己往前跑，这是灾难性的。

不用 RM 的话，pivot clip 的 180° 旋转和两段位移只能靠程序补：
- 旋转由 `PhysicsRotation` 补（已经是 0.17s）
- 位移只能完全放弃 → 角色原地转身，clip 里"跑过去 — 跑回来"全变成原地动作

所以状态机方案的实际结果是"退化成纯旋转路径 + 一个原地转身动画"，比现在差。
**结论：保持蒙太奇，用 6.7 的接口在蓝图里控制起止点。**

---

## 7. 参数表（`ACTCharacterMovementComponent`）

| 参数 | 默认 | 作用 | 调坏的后果 |
|---|---|---|---|
| `SharpTurnAngleThreshold` | 45° | 超过此角开始提速 | 太小 → 直线微调也提速，过冲 |
| `SharpTurnInterpSpeed` | 1080 deg/s | 大角度旋转速度 | 太大 → 转身像瞬移，Warping 来不及衔接 |
| `PivotAngleThreshold` | 140° | pivot 的角度门 | 太小 → 90° 也播 pivot，动作过重 |
| `PivotMinSpeed` | 350 cm/s | pivot 的速度下限 | 太低 → 低速也会进 clip，RM 把速度拉上去 |
| `PivotMaxSpeed` | 650 cm/s | pivot 的速度上限 | **必须和 clip 的设计速度匹配**，否则进入瞬间速度突变 |
| `TurnStableRate` | 120 deg/s | "方向已稳定"的角速度上限 | 太大 → 绕圈也会触发（核心参数，别乱调） |
| `TurnStableDwell` | 0.1s | 稳定判定时长 | 太小 → 搓摇杆就触发 |
| `PivotCooldown` | 0.9s | pivot 最小间隔 | **必须 ≥ 蒙太奇时长**，否则连播 |
| `TurnBrakeStartAngle` | 60° | 开始减速的角度 | 太小 → 正常跑动一直掉速 |
| `TurnBrakeMinScale` | 0.15 | 180° 时保留的速度比例 | 太高 → 滑步；太低 → 转身太沉 |

---

## 8. 验收

1. **直线跑动左右微调**（±30°）：不减速、不播 clip、脚不打滑。
2. **跑动中 90° 转**：0.1 秒内转完，速度短暂下降后恢复，无滑步。
3. **跑动中 180° 反打**：减速 → 撑地 → 转身 → 重新加速，全程脚不打滑，无朝向回弹。
4. **连续快速反打**（1Hz 左右左右）：每 0.9 秒最多一次 pivot，其余为快速旋转，
   **不允许出现蒙太奇互相打断**。
5. **绕圈搓摇杆**（持续同向旋转）：**不应触发任何 pivot**，纯旋转跟随。
6. **翻滚中反打**：翻滚蒙太奇不被 pivot 打断（`HandlePivotRequested` 有 `IsAnyMontagePlaying` 守卫）。
7. **翻滚结束后朝向不回弹**（4.4 的漂移保护）。
8. **pivot 进入瞬间**：速度不能有突变（说明 `PivotMinSpeed/MaxSpeed` 窗口对了）。
9. **pivot 淡出后**：底下的状态机应该还在 `Cycle`，**不能是 `Stop`**（露出来一段停止动画就是 6.4 末尾那个坑）。
10. **冲刺中反打**：不播 pivot（被 `PivotMaxSpeed` 挡掉），走纯旋转路径，允许观感差一些。
11. **贴墙反打**：**不会穿墙**（碰撞正常生效），但会看到"转身成功了、位置没挪过去 + 脚滑"。
    这是已知限制，不是 bug；要不要加通路检查见 6.5 ③。

### 验收前必须先确认的资产事实

`PC_01_Jog_Pivot_L_180_Lfoot` 和 `_R_180_Rfoot` **必须带 180° 的 root yaw**。
打开资产 → 看 `Root Motion` 面板，或用 Animation Sequence 的 `Root Motion Mode` 检查。
如果不带：

- 蒙太奇播了但胶囊不转 → 看起来像"原地做了个转身动作然后瞬间被转 180°"
- 修法：在 clip 上勾 `Enable Root Motion`；或者退回到"只用旋转提速"，删掉 pivot 蒙太奇配置

同一个面板里顺便读出三个数，填进下面的结论：

| 要读的 | 用途 |
|---|---|
| clip 总时长 | `PivotCooldown` 必须 ≥ 它（默认 0.9s，若 clip 更长要跟着改） |
| 转身**前**的位移长度 | 6.5 ③ 通路检查的第一段 |
| 转身**后**的位移长度 | 6.5 ③ 通路检查的第二段 |
| 转身点处的 RM 速度 | `PivotMinSpeed/PivotMaxSpeed` 的中心值，默认按 500 设的 |

如果这三段位移加起来超过 2.5 米，说明这个 clip 是给"开阔场地急转"用的，
在窄道里必须先做通路检查，否则穿墙会很明显。

同理确认 `AM_PC_01_Roll` 是否带 RM（4.4 的回弹问题只在有 RM 时才会暴露）。
