# 「朝移动方向」模式下的旋转 / 急停 / 转身 —— 曲线驱动重构方案

> 前置文档：`Turn-180-Design.md`（v1：蒙太奇 root motion + 旋转提速）、`ABP-v1-Wiring.md`（ABP 拓扑）
> 目标：**时机与角度由动画曲线决定，旋转的"写入口"只有一处，C++ 退化为管道。**
> 适用：UE 5.8 + `AnimationWarping` + `AnimationLocomotionLibrary`（本项目已启用）

---

## 0. 一句话结论

**把「旋转」从"物理按角速度追一个目标角"改成"动画按曲线推一个增量"，
物理只保留两件事：意图朝向（零延迟）和位移（含由曲线反推的减速度）。**

具体三条主改动：

1. **权威收敛成一个枚举**：`EACTRotationAuthority { Locomotion, Animation }`。
   取代现在并存的 `HasAnimRootMotion()` / `bAnimYawDriven` / `bPivotRotationFrozen` / `bInstantYaw`
   四路早退分支。转移点只有两个（`AnimNotifyState` 的 Begin/End），不再每帧比对。
2. **pivot 从「蒙太奇 + root motion」降级为「状态机里的一个普通状态」**。
   代价是放弃 clip 自带位移（改用曲线化的速度包络近似），换来：不占 Slot、不会被
   `bStopAllMontages` 误停、`Speed` 不再是 root motion 的"假速度"、可被攻击状态机正常打断。
3. **曲线契约从 1 个量扩到 3 个量**：`Pivot_Yaw`（角度+节奏）、`Pivot_Window`（旋转时机）、
   `Pivot_Speed`（速度包络）。角度用系数 `k = 需求角 / clip 角` 缩放，一套 180° 资产覆盖 140°~180°。

---

## 1. 现状：三类各自的真实职责

注释里写的职责和代码实际做的事，有明显偏差。先对齐事实：

| 类 | 注释声称 | 实际承担 | 越界点 |
|---|---|---|---|
| `AACTCharacterBase` | "只做输入→意图和相机，不写任何动画逻辑" | 输入、相机、状态转发、**播 pivot 蒙太奇**（选左右/跳 Section/记 Active/提前停） | `ACTCharacterBase.cpp:80-218`，共 139 行 pivot 专用代码，且持有 `PivotMontageLeft/Right` 资产 |
| `UACTCharacterMovementComponent` | "胶囊体位移的唯一权威 / 旋转单一权威" | 位移、**旋转插值**、转向分级、**速度折扣**、pivot 判定、动画驱动朝向的 GT 消费端、步态参数换算、动画快照 | 旋转不是"单一权威"：`PhysicsRotation` 有 4 个让路分支（`CMC.cpp:137/157/165/172`），分别让给 root motion、冻结、曲线、瞬时对齐 |
| `UACTAnimInstance` | "故意做得很薄，没有 WT 逻辑" | 3 个原子槽（offset / active / seq） | 只传了**一个** float，语义（时机、包络、角度需求）全在外面，导致 CMC 只能"按帧硬跟" |

### 1.1 现在的旋转产业链（实际）

```
Input_OnMove → AddMovementInput
                    ↓
CMC::UpdateDesiredYaw      GetLastMovementInputVector → TargetYawAngle（滞后一帧）
                    ↓
CMC::UpdateTurnState       六门判定 → OnPivotRequested.Broadcast（离散事件）
                    ↓
Character::HandlePivotRequested → Montage_Play（L/R 各一份资产）
                    ↓
蒙太奇 root motion → 胶囊转动（CMC 让路）
                    ↓
出蒙太奇 → PhysicsRotation 用 FixedTurn 补残差（1080 deg/s）
```

这条链有 4 个"权威交接点"，每个都靠注释约束顺序（"必须放在 X 之后"），
这是当前实现最贵的地方 —— 不是性能，是**每次改动都要重新推理一遍顺序**。

---

## 2. 不足：逐条钉在代码上

### 2.1 旋转

| # | 问题 | 位置 | 后果 |
|---|---|---|---|
| R1 | 权威四路并存，`PhysicsRotation` 靠 4 个早退分支协调，顺序即语义 | `CMC.cpp:137/157/165/172`；`CMC.h:373-393` | 任一条被误改 → 角色永久不再转向且**不报错**；`AnimYawTimeout=0.25s` 看门狗就是为此存在的补丁 |
| R2 | 旋转只有"角速度"，没有"节奏"：`FixedTurn(Current, Target, 540~1080 * dt)` | `CMC.cpp:180-206` | 旋转完成时刻与 clip 的支撑脚落地帧无关 → 落地前胶囊已经转完，脚必然滑 |
| R3 | 角度被 clip 定死 180°，`TurnAngle` 只用于选左右 | `Base.cpp:91` | 需求 140° 也转 180°，退出时 `FixedTurn` 回拉一次，肉眼可见"甩一下" |
| R4 | 曲线通道语义太窄：只有一个 offset，没有"时机门" | `AnimInstance.cpp:86-94` | 无法表达"脚落地后才开始转"；只能整段线性跟，节奏被抹平 |
| R5 | 目标角来自 `GetLastMovementInputVector()`（上一帧被消费的输入） | `CMC.cpp:237-244` | 推杆那一帧读到 0 → `bInputDrivenThisFrame=false` → `TurnStableTimer` 清零，pivot 确认整体推后 1 帧起 |
| R6 | 判定只有"输入 vs 当前速度"的夹角，无前视、无入口脚 | `CMC.cpp:325-336` | 无法区分"刚反打"与"绕圈中"（只能靠 `TurnStableDwell` 硬等）；资产是按脚分的（`Pl0001_Run_Pivot_*_Lfoot/Rfoot`），选择信息根本不存在 |
| R7 | Sprint(800) 被 `PivotMaxSpeed=650` 挡掉，退化为 1080 deg/s 硬转 | `CMC.h:232-245` | 全速反打是滑步最严重的场景，偏偏走的是最差路径 |

### 2.2 移动急停

先确认物理事实（这决定了后面怎么改）：

- `BrakingFrictionFactor = 0`（`CMC.cpp:50`）→ `ApplyVelocityBraking` 里 `bZeroFriction = true`
  → 退化为**纯恒定减速** `Velocity -= BrakingDeceleration * dt`（引擎 `CharacterMovementComponent.cpp` 的 `ApplyVelocityBraking`，`bZeroFriction` 分支）。
- 超速时的处理：`bVelocityOverMax → ApplyVelocityBraking(...)`（引擎 `:3907-3915`），**减速速率仍是 `BrakingDecelerationWalking`**；
  加速分支才用 `GetClampedToMaxSize`（引擎 `:3940-3942`）。即：**cap 只能限制上限，不能制造减速速率。**

| # | 问题 | 位置 | 后果 |
|---|---|---|---|
| S1 | 减速只有一档常量：`BrakingDecelerationWalking = MaxSpeed / TimeToStop` | `CMC.cpp:101/577` | 减速律是线性的、且与停步动画无关；真实停步是"先缓后急"，前段必滑 |
| S2 | 速度包络（`TurnSpeedScale`）是"上限"不是"减速"，引擎跟不上陡降 | `CMC.cpp:572` + 引擎 `:3907-3915` | 曲线让 cap 从 500 掉到 75 用 0.25s 时，需要 1700 cm/s² 才追得上，而基础减速度只有 1666；曲线再陡一点（0.1s → 4250）就会**冲过头**：多走 1.2~1.5m。这才是"包络救不回单次反打"的真正原因（`Turn-180-Design.md §5.2` 只归因于时序） |
| S3 | 停止距离不可预测：ABP 用 `PredictGroundMovementStopLocation` 推，但它只按 `BrakingDecelerationWalking` 算 | `ABP-v1-Wiring.md §2`（Stop 状态）+ 引擎库 `AnimCharacterMovementLibrary` | 实际进入 Stop 时可能还在步态插值 / `over max` 分支，两条分支减速律不同 → 预测落点 ≠ 真实落点 → `DistanceMatchToTarget` 追错距离，停住那一下脚滑或提前停 |
| S4 | 急停与转身共用 `TurnSpeedScale`，pivot 时又要整体让路 | `CMC.cpp:372-378` | 一个参数背两种语义；`bPivotOwnsMotion` 让路只是把"进入瞬间速度跳变"按住（`Turn-180-Design.md §5.1` 自己承认是补丁） |

### 2.3 转身（pivot）

| # | 问题 | 位置 | 后果 |
|---|---|---|---|
| P1 | 蒙太奇 + RM 的固有代价：占 `DefaultSlot`、`bStopAllMontages` 坑、RM 期间 `Velocity` 被整体替换 | `Base.cpp:107` | `LocomotionState.Speed` 变成 clip 的设计速度（"假速度"）；贴墙时旋转成功但位移失败（`Turn-180-Design.md §6.5③`） |
| P2 | 触发与执行跨三个对象（CMC 判定 → Character 播 → ABP 放） | 全链路 | 蓝图若再绑一次 `OnPivotRequested` 会同帧播两次；动作优先级只能靠 `IsAnyMontagePlaying()` 一刀切 |
| P3 | 冷却必须手工 ≥ clip 时长 | `CMC.h:266-268` | clip 重导变长 → 连播；变短 → 冷却过长 |
| P4 | 无"进入段距离匹配" | — | 进入速度与 clip 设计速度不匹配时，第一帧速度突变（RM 硬拉） |
| P5 | 资产维度只有"左/右" | `Base.h:229-233` | 无法表达 步态 × 方向 × 入口脚 × 角度，而项目里已有 4 个方向的资产（`Pl0001_Run_Pivot_B_F/F_B/LR_RR/RL_LL`） |

---

## 3. 行业做法（联网核对，含引擎事实）

| 方案 | 旋转由谁写 | 角度如何适配 | 时机来源 | 关键节点/函数 |
|---|---|---|---|---|
| **ALS-Refactored**（本项目已依赖） | 角色朝向由 CMC 平滑转向；转身动画只做表现 | 用 `RotationAmount` 曲线 × 播放速率折算 | clip 固定 | 转身动画里有"每帧旋转量"曲线，播完后角色已到位 |
| **Lyra** | Actor 跟 Controller 转；`Rotate Root Bone` + `Root Yaw Offset` 反向抵消，让网格"看起来没转" | 只播 90°，剩余靠 AimOffset | 状态机 + 静态切换 | `RootYawOffsetMode`：`Accumulate` / `Hold` / `Blend Out` |
| **GASP（UE5.7/5.8 官方示例）** | 胶囊由 Mover 转向；**根骨骼旋转完全由 root motion 控制** | `Steering` 节点按 `TargetOrientation` **缩放**动画旋转（`Min/MaxScaleRatio`）+ 弹簧补偿 | `FutureFacingDelta>50°` 触发、`Offset Root Bone` 兜住网格 | `Offset Root Bone(Accumulate)`、`Steering`（实验）、`Foot Placement`（`DisableLockCurveName`）、`IsPivoting` / `FutureFacingDelta` / `MovementDirectionRecent` 三参数选资产 |
| **Paragon Feature Examples** | — | **Distance Matching 做旋转**：按"还需转多少度"匹配动画时间点 | 曲线 | `AdvanceTimeByDistanceMatching`（把"距离"换成"角度"） |

引擎侧事实核对（5.8，本项目路径 `E:/UnrealEngine/UE_5.8`）：

- `UAnimDistanceMatchingLibrary::AdvanceTimeByDistanceMatching / DistanceMatchToTarget / SetPlayrateToMatchSpeed`
  都在 `AnimationLocomotionLibrary` 插件里，带 `BlueprintThreadSafe` → **Anim Node Function**，可在 AnimGraph 里当节点用。
- `UAnimCharacterMovementLibrary::PredictGroundMovementStopLocation / PredictGroundMovementPivotLocation`
  同样是 `BlueprintThreadSafe` 的纯函数；后者专门用来预测 **pivot 落点**。
- `FAnimNode_RotateRootBone`、`FAnimNode_OffsetRootBone`（`EOffsetRootBoneMode`，`MaxRotationError` / `RotationHalfLife`）、
  `FAnimNode_Steering`（`TargetOrientation` / `AnimatedTargetTime` / `Min-MaxScaleRatio`）、
  `FAnimNode_FootPlacement`（`DisableLockCurveName` / `SpeedCurveName` / `LockType`）均可用。
- `UAnimNotifyState_MotionWarping` + `USkewWarpRootMotionModifier`（`bWarpRotation` / `WarpRotationTimeMultiplier`）
  可把 clip 的旋转扭曲到任意目标角 —— 这是方案 B 的基础。
- `UMotionExtractorModifier` 支持 `bUseCustomCurveName` + `CustomCurveName` + `MathOperation/Modifier`
  → 可以直接烘出我们要的曲线名和单位，不需要后处理。

**结论：本项目该走 GASP 的思路（曲线决定旋转），但不引入 Mover 与实验节点；
用「曲线 → ABP → 一次权威交接 → CMC 落位」实现同一效果，风险最低。**

---

## 4. 新设计

### 4.1 分层

```
┌─ 意图层  AACTCharacterBase ─────────────────────────────────┐
│  输入 → MoveIntentYaw（本帧、零延迟）/ 请求步态、站姿、动作      │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌─ 判定层  UACTCharacterMovementComponent ────────────────────┐
│  位移（唯一）· TargetYawAngle（唯一）· TurnRequest（离散请求） │
│  RotationAuthority：Locomotion | Animation                  │
└──────────────────────────────┬──────────────────────────────┘
                               │  Property Access（只读）
┌─ 表现层  ABP ───────────────────────────────────────────────┐
│  Turn 状态（Sequence Player，非蒙太奇）                      │
│  GetCurveValue → k 缩放 → 推 offset/window/speed             │
│  Foot Placement / Orientation Warping(权重切换)              │
└──────────────────────────────┬──────────────────────────────┘
                               │  ANS_PivotWindow（GT，Begin/End）
                               │  AnimInstance 原子槽（每帧）
                               └────────→ 回到判定层落位
```

**唯一的写入口**：胶囊朝向只在 `PhysicsRotation` 里被写。
动画要转胶囊，只有一条路：`ANS_PivotWindow` 申请权威 → 每帧推曲线 → `OnEnd` 交还。

### 4.2 曲线契约（这是新方案的核心）

| 曲线 | 生成方式 | 值域 / 单位 | 语义 | 谁读 |
|---|---|---|---|---|
| `Pivot_Yaw` | `MotionExtractorModifier`：`Bone=root`、`MotionType=Rotation`、`Axis=Z`、`Space=LocalSpace`、`bRelativeToFirstFrame=true`、`bAbsoluteValue=false`、`bUseCustomCurveName=true` | 有符号度，首帧 0，末值 ≈ ±180 | **旋转的角度与节奏**（哪一段转得快由它决定） | ABP `GetCurveValue` |
| `Pivot_Window` | 手 K（0 → 1 在支撑脚落地帧，ease-in 约 0.04s；结尾可回 0） | 0 / 1 | **旋转的时机门**：脚没落地不转 | ABP `GetCurveValue` |
| `Pivot_Speed` | `MotionExtractorModifier`：`MotionType=TranslationSpeed`、`Axis=XY`、`MathOperation=Division`、`Modifier=该资产的参考步速`（jog=500） | 0~1.2 | **速度包络**：替代 `TurnBrakeMinScale` 这类常数 | ABP `GetCurveValue` |
| `Pivot_Foot` | 手 K（-1 左脚 / 0 双支撑 / +1 右脚）或 `FootstepAnimEventsModifier` | -1/0/+1 | 入口脚：选资产 + 决定是否镜像 | ABP（也可离线转成 Asset 元数据） |
| `Distance` | `DistanceCurveModifier` | cm | 仅 `Stop`/`Start` 用（距离匹配） | Anim Node Function |

**读取位置的硬约束**：

- `UAnimInstance::GetCurveValue` 内部走 `GetProxyOnGameThread<FAnimInstanceProxy>()`，
  **只能在 GT 上调**（ABP 的 Event Graph / 状态机 `On Update` / `On State Update`）。
  **不能**放进 `BlueprintThreadSafeUpdateAnimation` —— 那里调会 assert。
- GT 读到的是**上一帧评估完成**的曲线，滞后 1 帧（16ms）。对 0.5s 的转身是 3% 误差、恒定滞后不抖动，
  可以接受；消除它需要把胶囊旋转挪到动画之后，属于另一套架构（让动画驱动物理），不做。
- 想"零滞后"，只能改走 AnimGraph 内部的 `Modify Curve` 系列 —— 但那改的是**网格**，不是胶囊，
  等于变成方案 C（网格侧旋转），见 §8。

### 4.3 旋转：角度来自 k，时机来自 Window

```
需求角  θ_req = NormalizeAxis(DesiredYaw - ActorYaw)        // 窗口开始时冻结
clip 角 θ_clip = 资产元数据（Pivot_Yaw 末值，如 -180）
缩放系数 k = Clamp(θ_req / θ_clip, 0.55, 1.45)

每帧推送：
  Offset(τ) = Pivot_Yaw(τ) * k * saturate(Pivot_Window(τ))
  胶囊 yaw  = EntryYaw + Offset(τ)
```

三个要点：

1. **乘 `Window` 而不是只乘 `k`**。`Pivot_Yaw` 本身在落地前就有一小段漂移（尤其是 k<1 时被放大），
   不加门会"提前转身"。`Window` 把"能不能转"和"怎么转"拆成两个可独立调的量。
2. **k 超出 [0.55, 1.45] 就说明选错资产**（比如要转 60°，却拿了 180° 的 clip）。
   这时不要硬压 —— 换成"不提权"：`TurnRequest` 再判一次，小角度走 `Locomotion` 的平滑旋转。
3. **不做二次插值**。曲线本身含"慢-快-慢"，再 `FixedTurn` 一次会把中段抹平成匀速（`Turn-180-Design.md §9.2` 的结论仍然成立）。

### 4.4 退出：不提交也能对齐

`EndPivotWindow` 里**不需要写回目标角**：

- `UpdateDesiredYaw` 每帧都在把 `TargetYawAngle` 刷成 `DesiredYaw`（输入意图）；
- 曲线末值 = `EntryYaw + k*θ_clip ≈ EntryYaw + θ_req ≈ DesiredYaw`；
- 所以交还那帧的残差天然 < 3°，`Locomotion` 分支一次 `FixedTurn` 就抹掉，看不到"回拉"。

**这是放弃"提交角度"这个动作的前提**：`θ_req` 必须在窗口开始时冻结，
窗口内不允许玩家输入把 `DesiredYaw` 再改掉 —— 所以 `bAutoUpdateYaw` 在窗口内为 false，
`OnEnd` 时恢复。玩家窗口内继续转视角造成的残差，交还后由 `Locomotion` 按 1080 deg/s 补，最多 0.1s。

### 4.5 急停：让减速度跟上曲线

这是 v1 方案（`Turn-180-Design.md`）完全没解决的一块。三段改动：

**(a) 曲线陡降时，动态抬减速度（核心，CMC 内 3 行）**

```cpp
// 包络越陡，需要的减速度越大。基础 BrakingDecelerationWalking 追不上就会"冲过头"。
const float ScaleSlope = DeltaTime > KINDA_SMALL_NUMBER
    ? FMath::Abs(AnimSpeedScale - PrevAnimSpeedScale) / DeltaTime : 0.0f;
BrakingDecelerationWalking = FMath::Max(BaseBrakingDeceleration, SmoothedMaxSpeed * ScaleSlope);
```

算一遍就明白为什么必须有：Run=500，`Pivot_Speed` 在 0.1s 内从 1.0 掉到 0.15
→ 需要 `500 * 0.85 / 0.1 = 4250 cm/s²`；基础值 `500/0.3 = 1666`。
差 2.5 倍 → cap 掉到 75 时实际速度还有 333，多冲 1.2m 才收住。这就是"转身时冲出去一段"的量化原因。

**(b) 停止距离由 CMC 授权，ABP 只消费**

`LocomotionState.PredictedStopDistance`（新字段）= 用**与 `CalcVelocity` 相同的规则**积分出的制动距离
（考虑 cap、当前步态、`BrakingDecelerationWalking`）。ABP 的 `Stop` 状态直接喂给
`DistanceMatchToTarget(SeqEval, PredictedStopDistance, "Distance")`，
不再用 `PredictGroundMovementStopLocation`（它假设纯 `BrakingDecelerationWalking`，与实际分支不符，见 S3）。

**(c) `PlayRateClamp` 收窄到 (0.85, 1.15)**

默认 (0.75, 1.25) 会把停步动画拉成 1.25 倍速，脚频明显变快；
超出 15% 的部分交给 `Foot Placement` 和 `Stride Warping` 吸收，观感更稳。

### 4.6 转身触发：判定仍然用速度参照，但补两个信息

保留 v1 最值钱的那条结论（`Turn-180-Design.md §3.2`）：**判据的参照系必须是速度，不是朝向**。
在此基础上补：

| 新增 | 算法 | 用途 |
|---|---|---|
| `DesiredYaw` 零延迟 | `Input_OnMove` 里直接算世界方向角缓存（不再等 `GetLastMovementInputVector`） | 消除 R5 的 1 帧滞后；`bInputDrivenThisFrame` 改为看 `MoveIntent` 模长 |
| `θ_req` 冻结 | 候选成立那一帧记 `NormalizeAxis(DesiredYaw - ActorYaw)` | 给 `k` 用；也替代了"冻结旋转"补丁 —— 旋转本来就还没开始 |
| `EntryFoot` | 从 `Pivot_Foot` 曲线（jog cycle 里也要打）或 `Foot Placement` 的锁定脚反查 | 选 `_Lfoot` / `_Rfoot` 资产，或用 `Mirror Data Table` 镜像 |
| `Cooldown` 来源 | `TimeSinceTurnStateExited`（ABP 侧算，Property Access 可读） | 不再要求"冷却 ≥ clip 时长"手工对齐（P3） |

`TurnRequest` 结构（CMC 输出，ABP 只读）：

```cpp
USTRUCT(BlueprintType)
struct FActPivotRequest
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool  bPending = false;     // 本帧是否请求进入 Turn
	UPROPERTY(BlueprintReadOnly) float RequiredYaw = 0.0f;   // 需求角（有符号，度）
	UPROPERTY(BlueprintReadOnly) float SideSign = 1.0f;      // +1 左 / -1 右（= Sign(RequiredYaw)）
	UPROPERTY(BlueprintReadOnly) float DemandAngle = 0.0f;   // |输入 vs 速度|，判定用
	UPROPERTY(BlueprintReadOnly) bool  bSpeedInWindow = false;
};
```

---

## 5. 三个类的改动点与职责调整

### 5.1 `AACTCharacterBase` —— 退化成纯意图层

| 动作 | 内容 | 理由 |
|---|---|---|
| 删 | `PivotMontageLeft/Right`、`PivotPlayRate`、`ActivePivotMontage` | pivot 改成状态机状态后不再需要蒙太奇；资产选择交给 ABP，加一个步态/方向不用改 C++ 字段（P5） |
| 删 | `HandlePivotRequested`（含 `_Implementation`）、`PlayPivotFromSection`、`PlayMontageFromTime`、`StopPivotMontage`、`GetPivotMontagePosition`、`PostInitializeComponents` 里的 `AddDynamic` | 共 139 行；同时消灭"蓝图重绑导致同帧双播"的坑（P2） |
| 加 | `Input_OnMove` 里缓存 `MoveIntentYaw`（`UPROPERTY(BlueprintReadOnly)`，世界空间，度） | 零延迟意图（R5），Property Access 也能直接读 |
| 留 | 输入、相机、`SetGait/SetStance/SetRotationMode` 转发、`GetLocomotionState` | 职责不变 |

> 注意：**`RollMontage` 保留**。翻滚仍是蒙太奇（它是离散的、不需要每帧改朝向的动作），
> 只是 pivot 从蒙太奇里搬走了。

改造完成前先做了一轮死代码清理（见附录 A），这个类已从 474 行降到 399 行，全部删完后约 300 行，
且不再引用 `UAnimMontage`（除了 roll）。

### 5.2 `UACTCharacterMovementComponent` —— 只做"判定 + 落位"

| 动作 | 内容 | 理由 |
|---|---|---|
| 改 | 新增 `EACTRotationAuthority`，`PhysicsRotation` 从"4 个早退分支"改成"1 个 switch + 1 个小角度保护" | R1。分支总数从 4 降到 2，且不再有顺序依赖 |
| 改 | `RotationAuthority == Locomotion` 时，`|Residual| > MaxAutoTurnAngle(45°)` 直接 return（不硬转） | R2/R7。大角度不再由物理"甩"过去，等动画或走 `Turn` 状态；Sprint 反打退化为"慢一点转"而不是"瞬间甩" |
| 删 | `bPivotRotationFrozen`、`PivotFreezeTimer`、`TurnSpeedScale`、`TurnBrakeStartAngle`、`TurnBrakeMinScale`、`SharpTurnInterpSpeed`、`bAnimYawDriven`、`AnimYawTimeout` 的看门狗语义 | 冻结被 `Pivot_Window` 取代（数据驱动）；折扣被 `Pivot_Speed` 取代（曲线驱动）；权威被枚举取代 |
| 改 | `UpdateTurnState` 只填 `FActPivotRequest`，不广播、不冻结、不算折扣 | 判定与执行解耦；CMC 不再"请求播放"（P2） |
| 加 | `BeginPivotWindow(float RequiredYaw)` / `EndPivotWindow()`（幂等）/ `PushAnimSpeedScale(float)` | 权威交接的三个入口，共 ~50 行 |
| 加 | `LocomotionState.PredictedStopDistance` | S3 |
| 加 | `AnimSpeedScale` 差分 → 动态 `BrakingDecelerationWalking` | S2，3 行 |
| 留 | `SetTargetYawAngle`、`GetTurnAngle`、`GetRotationInterpSpeed`、步态/站姿、`LocomotionState` 快照 | 战斗锁定朝向等仍要用 |

`PhysicsRotation` 改造后（骨架）：

```cpp
void UACTCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (!HasValidData() || !UpdatedComponent) { return; }

	// 唯一的"外部改了朝向"修补。传送 / SetActorRotation / 蓝图硬改都走这里。
	const float ActorYaw = CharacterOwner->GetActorRotation().Yaw;
	if (FMath::Abs(FRotator::NormalizeAxis(ActorYaw - CurrentYawAngle)) > 1.0f)
	{
		CurrentYawAngle = ActorYaw;
	}

	// 唯一的分叉。没有第二个 bool 会在这里让路。
	if (RotationAuthority == EACTRotationAuthority::Animation)
	{
		// 曲线说了算：字面落位，不插值（曲线自带节奏）。
		CurrentYawAngle = FRotator::NormalizeAxis(AnimDrivenYaw);
		MoveUpdatedComponent(FVector::ZeroVector, FQuat(FRotator(0.0f, CurrentYawAngle, 0.0f)), false);
		return;
	}

	if (bInstantYaw)
	{
		bInstantYaw = false;
		CurrentYawAngle = TargetYawAngle;
		MoveUpdatedComponent(FVector::ZeroVector, FQuat(FRotator(0.0f, CurrentYawAngle, 0.0f)), false);
		return;
	}

	const float Residual = FRotator::NormalizeAxis(TargetYawAngle - CurrentYawAngle);
	if (FMath::Abs(Residual) > MaxAutoTurnAngle) { return; }   // 大角度交给动画，物理不硬甩

	const float InterpSpeed = GetRotationInterpSpeed();
	if (InterpSpeed <= 0.0f) { return; }

	const float NewYaw = FMath::FixedTurn(CurrentYawAngle, TargetYawAngle, InterpSpeed * DeltaTime);
	if (FMath::IsNearlyEqual(NewYaw, CurrentYawAngle, 0.01f)) { return; }

	CurrentYawAngle = FRotator::NormalizeAxis(NewYaw);
	MoveUpdatedComponent(FVector::ZeroVector, FQuat(FRotator(0.0f, CurrentYawAngle, 0.0f)), false);
}
```

`BeginPivotWindow` / `EndPivotWindow`：

```cpp
void UACTCharacterMovementComponent::BeginPivotWindow(float RequiredYaw)
{
	if (!CharacterOwner) { return; }

	RotationAuthority = EACTRotationAuthority::Animation;
	bAutoUpdateYaw = false;                       // 窗口内输入不许改目标角
	PivotRequiredYaw = FRotator::NormalizeAxis(RequiredYaw);

	AnimDrivenEntryYaw = CharacterOwner->GetActorRotation().Yaw;
	AnimDrivenYaw = AnimDrivenEntryYaw;
	CurrentYawAngle = AnimDrivenEntryYaw;
}

void UACTCharacterMovementComponent::EndPivotWindow()
{
	if (RotationAuthority != EACTRotationAuthority::Animation) { return; }   // 幂等：Notify OnEnd 与 State Exit 都会调

	RotationAuthority = EACTRotationAuthority::Locomotion;
	bAutoUpdateYaw = true;

	// 不写回 TargetYawAngle：下一帧 UpdateDesiredYaw 会把它刷成输入意图，
	// 而曲线末值 ≈ 输入意图，残差 < 3° 由 Locomotion 一次 FixedTurn 抹掉。
	CurrentYawAngle = CharacterOwner->GetActorRotation().Yaw;
	AnimSpeedScale = 1.0f;
}
```

### 5.3 `UACTAnimInstance` —— 只做通道，不解释语义

| 动作 | 内容 | 理由 |
|---|---|---|
| 改 | 三个原子槽 → 一个 `FPivotDriverSlot { float Offset; float Window; float Speed; uint32 Seq; }`，一次 `PushPivotDriver(Offset, Window, Speed)` | 一次原子写三值不现实（要保证撕裂安全），改成"各自 relaxed + Seq 统一自增"，GT 侧读 Seq 判断一致性 |
| 改 | `SetAnimationDrivenYawOffset` → `PushPivotDriver`；`StopAnimationDrivenYaw` → `ClearPivotDriver`（语义更准确） | 名字不再暗示"这就是旋转"，它只是通道 |
| 删 | 整套蒙太奇 MPSC 队列（`FActPendingMontage`、`PlayMontageSafe`/`QueueMontage`/`StopMontagesSafe`、`FlushPendingMontages`、`NativeUpdateAnimation` 覆写） | 全项目 0 引用（附录 A）；roll/attack 都走 Character 的 `PlayAnimMontage`，没经过这条 WT 安全通道。将来真需要再从 git 拿回来 |
| 不加 | 任何"选资产 / 算角度 / 判时机"的逻辑 | 全部在 ABP；AnimInstance 一旦开始判断，就会变成第二个 CMC |

> 为什么仍然需要 `UACTAnimInstance` 做通道：ABP 在状态机 `On State Update` 里是 GT，
> 但**写 `UPROPERTY` 会碰 GC/属性链**，且我们希望 WT 分支（如果将来把这段挪进线程安全更新）
> 也不会崩。原子槽 + Seq 是成本最低的跨线程契约，而且已经是现成的。

### 5.4 职责矩阵（改造后）

| 关注点 | 唯一负责人 | 其他人 |
|---|---|---|
| 输入意图 | Character | CMC 只读 |
| 位移 / 减速度 | CMC | ABP 只读（`PredictedStopDistance`） |
| 胶囊朝向的写入 | CMC 的 `PhysicsRotation` | 全部只读 |
| 何时开始转（时机） | ABP（`Pivot_Window` 曲线 + Notify） | CMC 只提供窗口 |
| 转多少度 | ABP（`k` = 需求角/clip 角） | CMC 提供 `RequiredYaw` |
| 转多快（节奏） | 动画资产（`Pivot_Yaw` 曲线） | ABP 只缩放，不改时序 |
| 速度包络 | 动画资产（`Pivot_Speed` 曲线） | ABP 推送，CMC 施加 |
| 播哪个资产 | ABP（Select：步态 × 侧 × 入口脚） | Character 不再知道有哪些 pivot 资产 |

---

## 6. 实现步骤

> 顺序很重要：**先烘曲线，再搭状态，最后才开权威**。中间任何一步都可以回退，不会出现"角色不转了"的黑盒状态。

### 步骤 0 · 资产准备（0.5 天）

1. pivot 资产现在是 `AnimSequence`（`PC_01_Jog_Pivot_L_180_Lfoot` / `Pl0001_Run_Pivot_*`），
   **不需要建蒙太奇** —— 状态机里的 Sequence 也能挂 Notify。这一点和 v1 方案相反，是本方案省事的地方。
2. 确认每个资产都**不带** root motion（或在导入设置里关 `Enable Root Motion`）。
   状态机播放时 RM 本来就被忽略，但留着会让 `Pivot_Yaw` 的烘焙值重复（root 转动被算两次）。
3. 左右镜像：给每个资产建对应的 `Mirror Data Table`（`Pl0001_Run_Pivot_B_F_Lfoot` ↔ `_Rfoot`），
   这样"左转 + 右脚支撑"这类组合可以靠 `Sequence Player` 的 `bMirrored` 得到，不用做 4 份资产。

### 步骤 1 · 烘曲线（0.5 天，批量）

对每个 pivot 资产右键 → `Animation Modifiers` → `Add Modifiers` → `MotionExtractorModifier`，加两个：

| 曲线 | Bone | Type | Axis | Space | RelativeToFirstFrame | Absolute | Custom Name |
|---|---|---|---|---|---|---|---|
| `Pivot_Yaw` | `root` | `Rotation` | `Z` | `LocalSpace` | ✅ | ❌ | ✅ `Pivot_Yaw` |
| `Pivot_Speed` | `root` | `TranslationSpeed` | `XY` | `LocalSpace` | — | — | ✅ `Pivot_Speed`，`MathOperation=Division`，`Modifier=500`（该资产参考步速） |

再手工加一条 `Pivot_Window`：
在支撑脚完全落地那一帧设 1，前一帧 0（切换用 0.04s 的 ease，避免 0/1 突变的数值跳变），
结尾最后 2~3 帧可以回 0（让"收尾"不参与旋转）。

**验收曲线**：打开 Sequence 看 `Pivot_Yaw`，应满足
① 首帧值 = 0；② 末值 ≈ ±180（或该资产的真实转角）；③ 中段最陡处的斜率对应"撑地那一下"；
④ 全段单调（不来回抖）。任一条不满足就说明 root 骨有额外动画或 `Space` 选错。

### 步骤 2 · CMC 最小改动（1 天）

按 §5.2 改。**这一步先把 `MaxAutoTurnAngle` 设成 180（等于不生效）**，
这样行为与现在完全一致，可以单独提交、单独验证不回归。

### 步骤 3 · ABP 加 `Turn` 状态（1.5 天，先不接管旋转）

1. `Locomotion_SM` 的 `Grounded` 父状态机下加 `Turn` 状态。
2. 进入条件：
   ```
   CharMoveComp.PivotRequest.bPending
   && !IsAnyMontagePlaying()                    // roll / attack 期间不转身
   && CharMoveComp.LocomotionState.Mode == Grounded
   ```
3. 状态内播放器：`Select` 链 `(bMirrored, Sequence)` → `Sequence Player`（`Loop=false`，`Start Position=0`），
   `bMirrored` 来自 `Pivot_Foot` 与 `SideSign` 的组合。
4. **这一步先不调 `BeginPivotWindow`**，只是让动画播出来、肉眼对照：动画转身 vs 胶囊转身的错位有多少。
   这个错位就是后面要消除的量。

### 步骤 4 · 打开权威交接（1 天）

1. 在 pivot 资产的 `Pivot_Window` 上升沿位置放一个 `AnimNotifyState`：
   新建蓝图类，父类选 `AnimNotifyState`，命名 `ANS_PivotWindow`。
   - `Received_NotifyBegin`：`CharMoveComp → Begin Pivot Window (RequiredYaw = PivotRequest.RequiredYaw)`
   - `Received_NotifyEnd`：`CharMoveComp → End Pivot Window`
   - 不实现 `NotifyTick`（旋转由 `On State Update` 推，避免两处写）
2. `Turn` 状态的 `On State Update` 里计算并推送：
   ```
   θ_req  = CharMoveComp.PivotRequest.RequiredYaw          // 窗口开始时读一次，缓存成变量
   k      = Clamp(θ_req / PivotClipAngle, 0.55, 1.45)      // PivotClipAngle 建议做成 DataAsset 元数据
   Offset = GetCurveValue("Pivot_Yaw") * k * Saturate(GetCurveValue("Pivot_Window"))
   Push Pivot Driver (Offset, Window, Speed = GetCurveValue("Pivot_Speed"))
   ```
3. `On State Exit`：`Clear Pivot Driver` + `End Pivot Window`（`Will Be Done`/`Completed` 两个出口都要连，
   `EndPivotWindow` 幂等，重复调没副作用）。
4. 把 `MaxAutoTurnAngle` 从 180 调到 **45**。此刻大角度旋转完全由曲线接管。

### 步骤 5 · Warping 权重切换（0.5 天）

`Turn` 状态里：
- `Orientation Warping` 的 `LocomotionAngle` 走一个 `Select`：`Turn` 中给 0（否则它会把腿扭回旧方向，
  表现为"转了一半，腿是斜的"，`Turn-180-Design.md §9.5` 已记录）。
- `Foot Placement` **保留并加强**：`DisableLockCurveName` 指向 `Pivot_Window` **取反**的曲线
  （窗口内允许锁脚），`LockType = PivotAroundBall`。这是"支撑脚不滑"的唯一来源。
- `Stride Warping` 在 `Turn` 中权重 0（这一步的步幅由 clip 决定，不该被速度再缩放）。

### 步骤 6 · 速度包络接上（0.5 天）

`PushPivotDriver` 的第三个参数接到 CMC 的 `AnimSpeedScale`，
`UpdateGaitParameters` 里把 `MaxWalkSpeed = SmoothedMaxSpeed * AnimSpeedScale`，
并加上 §4.5(a) 的动态减速度。验收：`Pivot_Speed` 最陡处（通常 0.1~0.15s 内从 1.0 掉到 0.2）
不应出现"冲出去再停"。

### 步骤 7 · 急停（0.5 天）

1. CMC 填 `LocomotionState.PredictedStopDistance`。
2. ABP 的 `Stop` 状态：`Sequence Evaluator` + `DistanceMatchToTarget(DistanceToTarget = PredictedStopDistance, "Distance")`，
   `PlayRateClamp = (0.85, 1.15)`。
3. `Stop → Idle` 的条件改成 `Speed < 5 && TimeRemaining < 0.05`（不再用固定时间）。

### 步骤 8 · 收尾

- 删 `AACTCharacterBase` 的 pivot 蒙太奇代码（§5.1），删 CMC 里被取代的字段（§5.2）。
- 把 `Turn-180-Design.md` 的 §6/§7 标注为"已被本方案取代"（避免后人按旧文档接线）。

### 步骤 9 · 调试与验收

- **Rewind Debugger** 是最有效的工具：把 `Pivot_Yaw*k`、`Offset`、`Residual`、`Speed` 都记进
  ABP 变量，在 Debugger 里对齐看"曲线何时开始动 / 胶囊何时开始转 / 速度何时掉"。
- 画三条箭头（照 GASP 的做法）：胶囊前向（黑）、root 骨前向（红）、目标朝向（品红）。
  本方案正常情况下后两条应该**重合**（因为网格没有独立旋转），一旦分开就说明有第二个写入者在改朝向。

---

## 7. 关键参数表

| 参数 | 位置 | 建议值 | 作用 | 调坏的后果 |
|---|---|---|---|---|
| `MaxAutoTurnAngle` | CMC | 45° | 超过此角物理不再插值，交给动画 | 太大 → 回到"硬甩"，脚滑；太小 → 90° 转向也要进 Turn 状态，动作过重 |
| `PivotAngleThreshold` | CMC | 140° | 进入 Turn 的判定门（输入 vs 速度） | 见 v1 文档，这条不变 |
| `PivotSpeedWindowMin/Max` | CMC | 350 / 650（jog） | 只在 clip 设计速度附近触发 | 不匹配 → 第一帧速度突变 |
| `TurnDemandLookahead` | CMC | 0.12s | 用输入角速度外推 0.12s 后的需求角，提前触发 | 太大 → 绕圈误触发；太小 → 反应慢 |
| `k` clamp | ABP | 0.55 ~ 1.45 | 角度缩放范围 | 超范围说明选错资产，应该换档位而不是硬压 |
| `k` 计算用 `PivotClipAngle` | DataAsset | 资产真实转角 | 分母 | 填错 → 整体角度偏 |
| `Pivot_Window` ease-in | 资产 | 0.04s | 旋转启动的软硬 | 0 → 起步有台阶；太长 → "慢半拍" |
| `BaseBrakingDeceleration` | CMC | `MaxSpeed/TimeToStop` | 基础减速 | 与 `TimeToStop` 联动，改一个要重算 |
| 动态减速度上限 | CMC | `SmoothedMaxSpeed * 4.0` | 防止曲线噪声把减速度顶到天上 | 不限 → 单帧尖峰当成真实斜率，角色"钉住" |
| `PlayRateClamp`（Stop） | ABP | (0.85, 1.15) | 停步动画的速率范围 | 放宽 → 鬼畜；收窄 → 脚滑 |
| `PivotCooldown` | ABP（`TimeSinceTurnExit`） | 0.55s | 连续反打的最小间隔 | 太长 → 第二次反打变硬转；太短 → 连播 |

---

## 8. 方案取舍

| | **A. 曲线权威交接（本方案）** | **B. 蒙太奇 + MotionWarping SkewWarp** | **C. 网格侧（Offset Root Bone）** | **D. 现状（蒙太奇 RM + 硬转）** |
|---|---|---|---|---|
| 旋转谁写 | CMC 落位曲线值 | root motion（被 warp 缩放） | 根骨骼（胶囊提前到位，网格抵消） | RM / `FixedTurn` 交替 |
| 角度可调 | `k` 连续（0.55~1.45） | warp 到任意目标角 | 连续 | 固定 180° |
| 时机来源 | `Pivot_Window` × 曲线 | Notify 窗口（时间） | 曲线 | clip 固定 + 冻结补丁 |
| 位移 | 速度 + 曲线包络（**近似**） | clip 原始位移（**精确**） | 速度 | clip 位移（精确） |
| 资产改动 | 烘 2 条曲线 + 手 K 1 条 | 只加 NotifyState | 烘曲线 + 实验节点 | 无 |
| 可被打断 | 状态机 transition（优先级清晰） | `IsAnyMontagePlaying` 一刀切 | 状态机 | 同 B |
| 已知坑 | 位移是近似的；曲线读 GT 滞后 1 帧 | 占 Slot、假速度、贴墙位移丢失、`bStopAllMontages` | `Offset Root Bone` 是实验节点；胶囊提前到位会让攻击判定/朝向提前生效 | 见 §2 |
| 适配本项目的程度 | ★★★★★ | ★★★ | ★★ | ★ |

**为什么选 A：**

1. **ACT 的本质是"高频、可打断、要参与动作优先级"**。蒙太奇方案里 pivot 与攻击的优先级只能靠
   `IsAnyMontagePlaying()` 判断，这是"谁先播谁赢"，不是优先级。状态机天然表达优先级。
2. **一套 180° 资产要覆盖 140°~180°**，只有"缩放曲线"这条路不需要新资产（B 也能，但 B 的旋转由 RM 写，
   缩放靠 warp，等于两套缩放机制并存）。
3. **位移是近似的，但对"撑地原地转身"类 clip 恰好不构成问题**：这类 clip 的根位移本来就接近 0，
   `Pivot_Speed` 曲线把它显式化之后，误差比"RM 位移 + 玩家输入叠加"更可控。

**什么时候该切到 B：**

- 某个 pivot 资产是"跑过去 → 转 → 跑回来"的三段式，位移 2m 以上，且必须精确落地（窄道、平台边缘）。
  这时用 B：`USkewWarpRootMotionModifier` 的 `bWarpRotation=true` + `WarpRotationTimeMultiplier`，
  把 clip 的 180° 扭曲到需求角；同时 `bEnableRootMotionRotation` 保持 true。
- 判据：`|clip 位移| > 80cm` 或"位移必须精确"的场景（攀爬、处决、翻越）。**不要全局切换**，
  按资产切（ABP 里 `Select` 一个 bool，或直接按资产类型走不同状态）。

**什么时候该切到 C：**

- 单机、表现优先、愿意用实验节点、且战斗判定不依赖胶囊朝向的精确时刻。
- C 的视觉质量最高（网格的旋转完全由动画决定，连"胶囊转早了"都看不见），
  代价是胶囊的朝向会领先动画 —— 对有锁定/朝向判定的 ACT 是隐患。

**为什么不继续 D：**

D 的 4 个让路分支已经到了"改一处要重推全链路顺序"的复杂度。继续在 D 上加曲线（v1 的 §9 方案），
只是在第 4 个分支里再插一条，边际收益递减，而调试成本线性上升。

---

## 8.5 方案 E：瞬转 + 刹车滑步（怪物猎人式）

> 触发这篇补充的观察：MH 的折返看起来"没有转身" —— 刹车滑行时身体已经面朝折返方向，
> 滑到折返点就继续跑。这不是"转得很快"，而是**根本没转**。

### 8.5.1 拆开看是三件事

| 量 | 行为 | 说明 |
|---|---|---|
| 胶囊朝向 | t≈0 就跳到新方向（1~3 帧） | `SetTargetYawAngle(DesiredYaw, bInstant=true)`，现有代码已支持 |
| 速度 | 沿**旧方向**衰减到 0，再沿新方向加速 | "折返点" = 速度过零那一帧，这是可观测的物理事实，不是美术约定 |
| 动画 | 播一条"面朝新方向、脚在拖"的 skid clip | 不是 run cycle，也不是带旋转的 pivot clip |

三者关系：**朝向领先于速度 30°~180°，整个滑行段角色都在"倒着滑"**。

### 8.5.2 为什么看不出"转"

1. **旋转窗口被压到 2~4 帧（33~66ms）**，低于"转"这个动作的感知阈值 —— 能读出的只有"他已经朝后了"。
   （动作游戏常见的 turnaround 甜点区就是 2~4 帧。）
2. **姿态变化比朝向慢**：blend 进 skid clip 要 0.06~0.12s，眼睛跟的是重心、四肢、武器，
   而不是 root yaw。
3. **滑步提供视觉锚点**：位移方向没变（还在往旧方向滑），画面里"没动"的是地面和脚 ——
   大脑据此判定"他在刹车"。**这一条是整套把戏成立的关键**：没有滑行，瞬转就是穿模式抽搐。
4. **支持性反馈**：扬尘、脚印、身体后仰、动作音效、运镜。MH 在这些上给得很足，
   观感贡献大于任何代码改动。

### 8.5.3 复刻到本项目：比方案 A 简单一个数量级

因为"旋转"这件事被删掉了，所以 `Pivot_Yaw`、`k`、`Pivot_Window`、权威交接全都不需要：

| 环节 | 做法 |
|---|---|
| 判定 | 不变：输入 vs 速度夹角 ≥ `PivotAngleThreshold` 且速度在窗口内 |
| 转向 | `SetTargetYawAngle(DesiredYaw, /*bInstant=*/true)` —— **一行**，走现有 `bInstantYaw` 分支 |
| 动画 | **复用 `Stop` 状态**，只是按入口角度换 clip（`Select`：`|TurnDemand| ≥ 140°` → skid 版） |
| 时长 | `DistanceMatchToTarget(PredictedStopDistance)`：滑行距离由物理决定，动画跟着对齐 |
| 曲线 | 只要 `Distance`（`DistanceCurveModifier`）+ 可选 `Skid_Brake`；不需要任何旋转曲线 |
| Warping | 滑行段 `Orientation Warping` / `Stride Warping` 权重必须为 **0**（`VelocityYawOffset≈180°`，>90° 会反向扭）；`Foot Placement` 保持开 |
| 减速 | 临时抬高 `BrakingDecelerationWalking`（×1.5~2.2）把滑行压到 0.25~0.35s；这是"滑多远"的主旋钮 |

一句话：**折返 = 瞬转 + 播 Stop**。状态机不需要新状态，CMC 不需要新字段。

### 8.5.4 什么时候不能这么干

- **低速**（`< PivotMinSpeed`）：没有惯性可滑，瞬转就是"原地闪"，比动画 pivot 更假 → 低速仍走 A。
- **大体型 / 大武器**：剪影越大，1~3 帧翻转越容易被看见。
- **需要"读招"的战斗**：魂系 / DMC 把可见转身当节奏的一部分（前摇、朝向承诺），瞬转会削弱重量感。
  MH 敢这么干，是因为它的读招压力在**怪物侧**，猎人侧优先响应。
- **镜头**：如果 spring arm 朝向跟随 actor，瞬转会甩镜头。本项目相机是 controller-driven，天然安全。

### 8.5.5 与方案 A 的关系：互补，不是替代

按 **角度 × 速度** 分档，一套参数表就能切换三种表现：

| 输入 vs 速度夹角 | 速度 | 处理 | 观感 |
|---|---|---|---|
| < 45° | 任意 | `Locomotion` 连续插值（现状 540/1080） | 顺滑微调 |
| 45°~140° | > 350 | 方案 A：曲线 pivot，`k` 压缩到 0.4~0.78 | 看得见的转身 |
| **≥ 140°** | **> 350** | **方案 E：瞬转 + skid** | **刹车折返，看不见转** |
| ≥ 140° | < 350 | 方案 A（现有 180° clip） | 原地转身 |

这也顺带解释了为什么方案 A 在 180° 反打上收益有限：**那个场景本来就不该有"转"这个动作**。
曲线驱动应该留给 45°~140° 这类"看得见、需要脚法"的转向。

---

## 9. 验收清单

1. **直线微调 ±30°**：不进 `Turn`，不播动画，`MaxAutoTurnAngle` 内由 `Locomotion` 处理，无滑步。
2. **90° 转向**：`Locomotion` 转一半 + 剩余残差 < 45° 收尾，或进 `Turn` 但 `k≈0.5`（此时该换 90° 资产）。不允许出现"瞬间甩 90°"。
3. **180° 反打**：动画播完时 `Residual < 3°`；胶囊无回拉；脚在撑地段不打滑。
4. **进入 Turn 那一帧**：`Speed` 无跳变（说明 `Pivot_Speed` 首值 ≈ 1，且 `PivotSpeedWindow` 匹配）。
5. **连续反打 1Hz**：`Turn` 状态不重叠；间隔由 `TimeSinceTurnExit` 控制；被拒的那次走 `Locomotion`（允许观感差，不允许抽搐）。
6. **绕圈搓摇杆**：一次 `Turn` 都不触发（速度参照 + `DemandAngle` 稳定窗口保证）。
7. **Turn 中按攻击**：按设计要么排队、要么打断；打断时 `EndPivotWindow` 必须被调用（幂等），
   否则角色永久不再转向（这是本方案唯一的"致命漏调"）。
8. **翻滚中反打**：不进 `Turn`。
9. **贴墙反打**：`Turn` 照常播，位移由 `Velocity` 提供 → 会被墙挡住，但**不会有"转身成功、位置没动"的错位**
   （因为不再有独立于位移的 RM 旋转），这是相对 D 的一个净改善。
10. **Sprint 反打**：`PivotSpeedWindowMax` 挡掉 → 走 `Locomotion`，`MaxAutoTurnAngle` 限制下不再"瞬间甩 180°"，
    而是被"上限"卡住（观感是"转得慢但稳"，可接受；要更好就补 sprint pivot 资产）。
11. **只狼式急停（松开摇杆）**：停住那一帧动画刚好到最后一帧（`DistanceMatchToTarget` 正确），无"停住后还在迈步"。
12. **低帧率（30fps）**：曲线在 GT 读、滞后 1 帧 → 33ms，转身中段角速度 ~600 deg/s 时滞后 20°，
    表现为"略微晚转"。这是本方案的已知代价，不接受就必须切方案 C。

---

## 10. 已知代价（明确写下来，避免以后当成 bug）

1. **滞后 1 帧**：曲线在 GT 读，16ms@60fps。窗口开始前曲线恒 0，所以只在"最陡那一段"可见。
2. **位移是近似的**：`Pivot_Speed` 曲线乘 `MaxWalkSpeed`，积分后 ≈ clip 位移但不保证相等；
   窄道/远景对位会露馅。
3. **残差收尾**：`k` 被 clamp 时（选错资产）残差会 > 3°，交还时由 `Locomotion` 按 1080 deg/s 补，
   最多看到 0.1s 的"补转"。这是"不提权"策略的兜底，不要试图消灭它。
4. **镜像的边界**：`Mirror Data Table` 不镜像 Notify 的参数（`RequiredYaw` 是运行时传的，不受影响），
   但会镜像曲线的**符号** —— 必须验证 `Pivot_Yaw` 镜像后符号正确，否则会"往反方向转"。
   验证方法：镜像后在 Sequence 里看 `Pivot_Yaw` 末值是否变号。

---

## 附录 A · 已执行的死代码清理

> 时间：2026-10-28。范围：本方案涉及的三个类。**只删"全项目 0 引用且新设计不需要"的东西**，
> 未触碰任何活跃路径（pivot 当前仍走蒙太奇 + root motion，等第 4 步替换）。

### A.1 审计方法

`uasset` 的 FName 名表是明文，因此可以拿它当"引用图"用：

```bash
# 名字出现在名表里 = 这个包历史上引用过它；没出现 = 一定没引用。
grep -ra "<Symbol>" Content | wc -l
```

关键的不对称性：**缺名能证明"没用"，有名不能证明"在用"**（删掉节点后名字会残留在名表里）。
所以只按"缺名"删。对照组用 `CameraBoom`（必然存在于 BP_PC_01）验证扫描有效性。

复核结论（都在 `Content` 里查过）：

| 符号 | 资产引用 |
|---|---|
| `HandlePivotRequested` | `BP_PC_01` —— **在用，保留** |
| `StopPivotMontage` | `ABP_PC_01` 名表 —— **保留** |
| `SetAutoYawUpdate` | `ABP_PC_01` + `BP_PC_01` —— **在用，保留** |
| `StopAnimationDrivenYaw` | `ABP_PC_01` —— **在用，保留** |
| `GetACTMovement` / `GetActCharacter` / `LocomotionState` | 40 / 10 / 29 处 —— **在用，保留** |
| 本附录 A.2 列出的全部符号 | 0 —— 删除 |

### A.2 已删除

| 类 | 符号 | 行数 | 为什么是死的 |
|---|---|---|---|
| `AACTCharacterBase` | `PlayMontageFromTime` | 24 | 0 引用；新设计不需要"从任意时间起播"（pivot 变状态机后由曲线决定进度） |
|  | `PlayPivotFromSection` | 38 | 0 引用；同 v1 文档 §6.4 记录的"蓝图重写没走这条路" |
|  | `GetPivotMontagePosition` | 11 | 0 引用；"转到就停"改由 `Pivot_Window` / Notify 表达 |
| `UACTCharacterMovementComponent` | `GetCurrentYawAngle` / `GetTargetYawAngle` | 9 | 0 引用；动画层改用 Property Access 读 `LocomotionState.YawOffset` |
|  | `IsSharpTurning` / `GetTurnAngle`（保留 `GetTurnAngle`）/ `GetTurnSpeedScale` | 8 | `IsSharpTurning`、`GetTurnSpeedScale` 0 引用，且语义被 `TurnRequest` / `Pivot_Speed` 取代 |
|  | `IsLanding` | 5 | 0 引用；文档一直让读 `LocomotionState.bLanding` |
|  | `IsAnimationDrivenYaw` | 5 | 0 引用；第 2 步会以 `GetRotationAuthority()` 重新出现（带语义） |
|  | `SmoothedTurnRate` + `TurnRateSmoothingRate` + 计算块 | 12 | **纯写不读**的"诊断用"量：算完写进一个没人读的字段，每帧白烧一次 `FInterpTo` |
| `UACTAnimInstance` | `FActPendingMontage`、`PendingMontages`、`PendingStopRequests`、`PlayMontageSafe`、`QueueMontage`、`StopMontagesSafe`、`FlushPendingMontages`、`NativeUpdateAnimation` 覆写 | 124 | 整套 MPSC 蒙太奇队列 0 引用。roll / attack 都走 `ACharacter::PlayAnimMontage`，从没经过这条 WT 安全通道 |

净减 **265 行**（2173 → 1908）：

| 文件 | 前 | 后 |
|---|---|---|
| `ACTCharacterBase.h` / `.cpp` | 252 / 474 | 226 / 399 |
| `ACTCharacterMovementComponent.h` / `.cpp` | 417 / 734 | 392 / 719 |
| `ACTAnimInstance.h` / `.cpp` | 165 / 131 | 123 / 49 |

### A.3 0 引用但**故意保留**

| 符号 | 理由 |
|---|---|
| `SetTargetYawAngle` | 攻击锁朝向、处决朝向是已设计好的用途（`bAutoUpdateYaw=false` 的配套），Phase 6 要用 |
| `SetRotationInterpSpeed` / `RotationInterpSpeedOverride` | 同上：临时冻结朝向的唯一入口 |
| `GetTurnAngle` | `TurnRequest.RequiredYaw` 的来源量；ABP 算 `k` 与调试覆盖层都要读，保留成本 6 行 |
| `bPivotRotationFrozen` / `PivotFreezeTimer` | 当前活跃路径（确认窗口内冻结旋转），**第 4 步由 `Pivot_Window` 取代后才删** |
| `PivotMontageLeft/Right`、`PivotPlayRate`、`ActivePivotMontage`、`HandlePivotRequested`、`StopPivotMontage` | pivot 现在**唯一可用**的实现，`BP_PC_01` 重写了它。删了 pivot 立刻失效 —— 必须等状态机版本跑通再删 |

### A.4 编译验证

`UnrealBuildTool ACTGameEditor Win64 Development`：

- **编译通过**：`ACTAnimInstance.cpp` / `ACTCharacterBase.cpp` / `ACTCharacterMovementComponent.cpp`
  以及 unity 重编的 `Module.ACTGame.cpp` 全部无 error（仅 `CombatStateTreeUtility.h` 的 C4996 旧 API 警告，与本次改动无关）。
- **链接未完成**：本机 UBA 本地执行器对所有输出文件报
  `SetFileInformationByHandle (FileDispositionInfo) failed ... (Access is denied.)`，
  导致 `.lib` / `.pdb` 被写成 **0 字节**，随后链接器报 `LNK1136 无效或损坏的文件` / `LNK1201 写入程序数据库出错`。
  出问题的不止 ACTGame，`UnrealEditor-ALS.lib`、`UnrealEditor-VisualStudioTools.lib` 同样被清零 ——
  属于**环境问题（UBA/权限），与代码无关**。

已把 11 个 0 字节产物清掉（`Intermediate` / `Binaries` / `Plugins` 下的 `.lib`/`.exp`/`.pdb`/`.dll`），
避免下次构建拿 0 字节库当"已是最新"而继续报 LNK1136。恢复方式：

1. 关掉所有 UnrealEditor / LiveCoding 进程；
2. 用 Visual Studio 打开 `ACTGame.sln`，`Build → Rebuild`（或先 `Clean` 再 `Build`）；
3. 若仍报 `LNK1201`，把 `mspdbsrv.exe` 加进杀软白名单，或临时用 `-NoLinkerDebugInfo` 构建。

> 注意：本次验证时用了 `-NoDebugInfo`，所以下次正常构建会触发一次全量重编（预期行为，无需手工清理）。

> 提醒：本次清理后 `UACTAnimInstance` 只剩下原子槽。如果将来动作系统确实需要"在 WT 上安全播蒙太奇"，
> 从 git 历史拿回 `FlushPendingMontages` 那套即可，不要重新设计。

---

## 附录 B · 重构：移动组件去掉 turn 逻辑（2026-10-28）

> 目标：组件回到三件事 —— **位移、朝向插值、动画快照**。
> 转身（判定 / 播什么 / 转多少 / 什么时候转）全部交给动画层，见 §4 与 §8.5。

### B.1 删除清单

| 类别 | 删除项 | 说明 |
|---|---|---|
| 事件 | `FACTPivotRequested`、`OnPivotRequested` | pivot 不再由物理侧"请求播放" |
| 判定 | `UpdateTurnState()` 整个函数 | 六门判定、稳定计时、冷却、速度折扣全部移除 |
| 判定数据 | `PivotAngleThreshold`、`PivotMinSpeed`、`PivotMaxSpeed`、`TurnInputSettleAngle`、`TurnStableDwell`、`PivotCooldown`、`PivotCooldownTimer`、`TurnStableTimer`、`PrevInputYaw`、`bHasPrevInputYaw`、`InputYaw`、`bInputDrivenThisFrame` | 只剩"目标角 = 输入方向"这一件事 |
| 旋转提速 | `SharpTurnAngleThreshold`、`SharpTurnInterpSpeed` + `PhysicsRotation` 里的角度插值 | 旋转只剩一种速度（按 RotationMode 取） |
| 冻结 | `bPivotRotationFrozen`、`PivotFreezeTimer` | 需要冻结时用 `SetRotationInterpSpeed(0)` |
| 速度包络 | `TurnSpeedScale`、`TurnBrakeStartAngle`、`TurnBrakeMinScale`、`TurnSpeedSmoothingRate`、`UpdateGaitParameters` 里的乘法 | 速度只由步态决定，不再被角度调制 |
| 动画驱动朝向 | `ConsumeAnimationDrivenYaw()`、`EndAnimationDrivenYaw()`、`bAnimYawDriven`、`AnimationDrivenYaw`、`AnimDrivenEntryYaw`、`AnimYawTimeout`、`AnimYawIdleTimer`、`LastAnimYawSeq` + `PhysicsRotation` 的曲线分支 + 对 `UACTAnimInstance` 的依赖 | 该通道日后由新的 `BeginPivotWindow/EndPivotWindow` 取代（§5.2） |
| 查询 | `GetTurnAngle()` | 语义属于"转身判定"，随判定一起走；新设计里是 `TurnRequest.RequiredYaw` |
| 角色侧 | `HandlePivotRequested`、`StopPivotMontage`、`PivotMontageLeft/Right`、`PivotPlayRate`、`ActivePivotMontage`、`PostInitializeComponents` 里的 `AddDynamic` | pivot 不再认识蒙太奇 |

行数：移动组件 **1111 → 678**（-433），角色 **625 → 504**（-121），合计 -554。

### B.2 保留（不是 turn 专属，或新设计还要用）

| 保留项 | 理由 |
|---|---|
| `SetTargetYawAngle(yaw, bInstant)` | 折返/翻滚/攻击起手的唯一转向原语（§8.5 折返就靠 `bInstant`） |
| `SetAutoYawUpdate` | 攻击锁定、折返窗口内冻结目标角；`ABP_PC_01` / `BP_PC_01` 都在调 |
| `SetRotationInterpSpeed` | 冻结旋转/临时加速的通用开关 |
| `GetTargetYawAngle` / `GetRotationInterpSpeed` | 上面两个 setter 的读回，成对保留 |
| `PhysicsRotation` 的 `HasAnimRootMotion` 分支与 1° 漂移修补 | 翻滚仍是蒙太奇；传送/蓝图改朝向仍要同步缓存 |
| `LocomotionState` 全部字段（含 `VelocityYawOffset` / `YawOffset`） | 动画层只读契约，字段没变，ABP 不用改接线 |

### B.3 行为变化（有意为之，验收时按这个对）

1. **不再自动播 pivot**：180° 反打现在只是"按 `VelocityDirectionInterpSpeed`(540°/s) 平滑转 0.33s"，
   期间靠 `BrakingDecelerationWalking` 减速。这就是方案 E 落地前的基线。
2. **大角度不再提速**：原来 45° 以上会插值到 1080°/s，现在统一 540°/s。想快就调
   `VelocityDirectionInterpSpeed`，或走 `bInstant`（瞬转）。
3. **转身不再掉速**：`TurnSpeedScale` 移除后，转身期间保持满速（脚会更滑）—— 这是把
   "减速"的职责显式交给动画层（`Pivot_Speed` 曲线或 skid 的 `Distance` 匹配）。
4. **朝向不再由曲线驱动**：`UACTAnimInstance` 的两个原子槽函数仍在（ABP 还调着
   `StopAnimationDrivenYaw`），但**已经没有消费方**，当前是空转；等 §5.2 的
   `BeginPivotWindow` / `PushPivotDriver` 落地后一并替换。

### B.4 资产侧要跟着改的地方

| 资产 | 需要做的 |
|---|---|
| `BP_PC_01` | `HandlePivotRequested` 覆盖已成为孤儿函数（父类没有它了），删除该覆盖；若之前给 `PivotMontageLeft/Right` 赋过值，对应节点会变成"未知属性"，一并删掉；`M_PC_01_Jog_Pivot_L_180_Lfoot` 的引用随之失效（资产本身留着，方案 E 还要用序列） |
| `ABP_PC_01` | `SetAutoYawUpdate` 仍保留，这一处不用改；`StopAnimationDrivenYaw` 已在附录 C 中删除，若图里存在该节点需删掉 |
| 蓝图 CDO | 之前调过的 `PivotAngleThreshold` 等参数会静默消失（属性已删），不会报错但也不再生效 |

### B.5 编译状态

本机编译未执行：编辑器正在运行（Live Coding 持有锁），UBT 直接拒绝：
`Unable to build while Live Coding is active`。关掉编辑器后执行：

```bat
"E:/UnrealEngine/UE_5.8/Engine/Build/BatchFiles/Build.bat" ACTGameEditor Win64 Development ^
  -Project="E:/UnrealEngine/Projects/ACTGame/ACTGame.uproject" -WaitMutex
```

> 上次链接残留的 0 字节产物已清理（附录 A.4），这次是完整重编 + 重新链接。

---

## 附录 C · 重构：角色类与动画实例去掉多余逻辑（2026-10-28）

> 前置：附录 B 已经让移动组件不再消费"动画驱动朝向"，也不再发出 pivot 请求。
> 那次改动留下两个悬空物 —— 角色类里指向已删函数的转发、动画实例里没人读的原子槽。本附录清掉它们。

### C.1 判定标准（和附录 A 的区别）

附录 A 是"0 引用就删"。这次不能只按引用数删，因为**没被引用的东西也分两种**：

| 类型 | 例子 | 处置 |
|---|---|---|
| **冗余层** —— 数据本来就能从别处拿到 | `GetLocomotionState()`（ABP 用 Property Access 直读）、`GetCameraBoom()`（本是 BlueprintReadOnly 属性） | 删 |
| **断头通道** —— 生产端/消费端已被删掉一半 | 原子槽整条链（读端 `ConsumeAnimationDrivenYaw` 已删） | 删 |
| **未接线功能** —— 代码完整，只是资产没配 | `Input_OnJump` / `Input_OnCrouch` / `Input_OnRoll` / `Input_OnWalk` | **保留**，见 C.4 |

判据补充：`BindAction(nullptr, ...)` 不会崩，只会生成一个永远匹配不上的 binding（`UEnhancedInputComponent::BindAction` 内联在头文件里，没有 null 检查）。所以"资产没配"不等于"代码是死的"，只是"当前不触发"。

### C.2 删除清单

**`UACTAnimInstance`（172 → 116 行）**

| 删除项 | 说明 |
|---|---|
| `SetAnimationDrivenYawOffset()` | 写端。ABP 名表里**根本不存在**这个名字 —— 从来没有节点调用过它 |
| `StopAnimationDrivenYaw()` | 写端。名表里有残留名字，但读端已删，调用也不会有任何效果 |
| `IsAnimYawDriveActive()` / `GetAnimYawDriveOffset()` / `GetAnimYawDriveSeq()` | 三个 GT 侧拉取接口，调用方 `ConsumeAnimationDrivenYaw` 已随附录 B 删除 |
| `AnimYawOffsetSlot` / `AnimYawActiveSlot` / `AnimYawSeq` | 三个 `std::atomic` 字段 |
| `#include <atomic>` | 上面三个没了就没人用 |

整条链的死亡路径很清晰：**写端（ABP）从未接过 → 中间原子槽 → 读端（CMC）已被删除**。三段里只剩中间一段，属于纯负债。

类头注释同步重写。原注释说"唯一剩下的职责：跨线程朝向通道"，现在改成"唯一的存在理由：给 ABP 一个 GT 侧的缓存访问点"，与实际职责对齐。

**`AACTCharacterBase`（504 → 489 行）**

| 删除项 | 引用情况 | 为什么是冗余层 |
|---|---|---|
| `GetCameraBoom()` / `GetFollowCamera()` | `Content` 里 `GetCameraBoom` **无名表**，源码 0 调用 | `CameraBoom`/`FollowCamera` 本身就是 `BlueprintReadOnly` 组件属性，蓝图拖出来直接用。这两个 getter 是 ALS Example 角色带过来的习惯写法 |
| `GetLocomotionState()` | `Content` 里**无名表**，源码 0 调用 | ABP 用 Property Access 读 `CharMoveComp.LocomotionState`；C++ 侧走 `GetACTMovement()->GetLocomotionState()`（返回引用，零拷贝）。这个按值拷贝的蓝图转发没有存在价值 |

### C.3 保留清单（0 引用，但删了会更糟）

| 保留项 | 理由 |
|---|---|
| `SetRotationMode()` / `GetRotationMode()` | 源码内部 0 调用、蓝图 0 引用，**但** `ABP_PC_01` 读了 `RotationMode`（名表 4 处）。删掉 setter，`LookingDirection` 分支就永久不可达 —— ABP 里的分支会变成死分支，比多 12 行代码更糟 |
| `SetGait` / `GetGait` / `SetStance` / `GetStance` | 蓝图 0 引用，但 `Input_OnSprint` / `Input_OnJump` / `Input_OnCrouch` / `Input_OnRoll` 都在用，是输入层的必要转发 |
| `GetACTMovement()` | `ABP_PC_01` 与 `BP_PC_01` 都在调 |
| `GetActCharacter()` | `ABP_PC_01` 在调，且是 WT 拿 self 之外唯一安全入口的前提 |
| `bDesiredWalk` | 只在 `Input_OnWalk` ↔ `Input_OnSprint` 之间传递，见 C.4 |

### C.4 保留的"未接线功能"（不是死代码，是资产没配）

审计时发现 `BP_PC_01` 的这四个属性**从未赋值**，且 `IMC_Default` 里也没有对应的 Input Action：

| 属性 | `BP_PC_01` 赋值 | IMC 绑定 | 后果 |
|---|---|---|---|
| `JumpAction` | ✗ null | `IA_Jump` 存在但未赋给角色 | 跳跃无响应 |
| `CrouchAction` | ✗ null | 无 `IA_Crouch` | 蹲下无响应 |
| `RollAction` | ✗ null | 无 `IA_Roll` | 翻滚无响应 |
| `WalkAction` | ✗ null | 无 `IA_Walk` | 走/跑偏好切换无效 |

已赋值的是：`InputMappingContext`、`MoveAction`、`LookMouseAction`、`LookAction`、`SprintAction`。

这四项**没有删**，因为代码本身是完整的（含蹲姿起跳先站起、翻滚前强制站立这类细节），删了以后要重写一遍。但需要知道：**当前它们全是静默失效状态**，不是 bug 也不是死代码。要接线就是补 `IA_` 资产 + IMC 映射 + BP 赋值。

> 顺带：`BP_PC_01` 被 `BP_ACT_GameMode` 引用，而 `DefaultEngine.ini` 的 `GlobalDefaultGameMode` 指向 `BP_ThirdPersonGameMode`。如果测试时用的不是 `BP_ACT_GameMode`，那看到的是模板角色而不是 PC_01。

### C.5 资产侧待办

| 资产 | 需要做的 |
|---|---|
| `ABP_PC_01` | 若图里存在 `StopAnimationDrivenYaw` 节点，删除（父类已无此函数，会编译报错）。名表里有这个名字的残留，但 `SetAnimationDrivenYawOffset` **完全没出现过** —— 说明写入端从来没接过 |
| `BP_PC_01` | 无新增改动；附录 B 提到的 `HandlePivotRequested` 孤儿覆盖仍需手工删 |

### C.6 构建验证（这次跑通了）

```
Build.bat ACTGameEditor Win64 Development -Project=...\ACTGame.uproject -NoUBA -NoLinkerDebugInfo
Result: Succeeded
Output: Binaries/Win64/UnrealEditor-ACTGame.dll (1,842,688 bytes)
```

- **编译 0 error**：`ACTCharacterBase.cpp` / `ACTAnimInstance.cpp` / `ACTCharacterMovementComponent.cpp` / `Module.ACTGame.cpp` 全部通过（仅 `CombatStateTreeUtility.h` 的 C4996 旧 API 警告，与本次无关）。
- 链接成功的条件是 **`-NoUBA`**：UBA 本地执行器在这台机器上对所有输出报
  `SetFileInformationByHandle (FileDispositionInfo) ... Access is denied`，把 `.lib`/`.pdb` 清零，
  随后链接器报 `LNK1136`（导入库损坏）/`LNK1201`（写 PDB 失败）。
  绕开 UBA 后链接正常，PDB 也正常生成（73 MB）。**日常构建遇到 LNK1136 就是 UBA 写入失败，不是代码问题。**

行数变化（基线取附录 B 结束时的状态）：

| 文件 | 附录 B 后 | 现在 | 差 |
|---|---|---|---|
| `ACTCharacterBase.h` + `.cpp` | 504 | 172 + 317 = 489 | **-15** |
| `ACTAnimInstance.h` + `.cpp` | 172 | 84 + 32 = 116 | **-56** |

本次合计 **-71 行**。`ACTAnimInstance` 的 56 行几乎全是删除（注释重写略有回补）；
`ACTCharacterBase` 只删了 3 个函数签名与实现，加上把重复的相机注释改成一段说明。

从附录 A 起算的累计：`ACTCharacterBase` 726 → 489，`ACTAnimInstance` 296 → 116，
移动组件 1111 → 678，三个类合计 **2133 → 1283（-850，-40%）**。
