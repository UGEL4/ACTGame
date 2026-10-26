# ABP v1 接线手册（节点优先）

> 配套文档：`ACT-Locomotion-Refactor-Plan.md`（整体方案）、`ALS-Refactored-Code-Study.md`（参考实现分析）
> 适用：UE 5.8 + `AnimationWarping` + `AnimationLocomotionLibrary` + 有 root motion 的动画包

---

## 0. 实现层归属原则

**节点能做的，一律在动画蓝图做；C++ 只保留节点做不到的。**

理由不是"少写代码"，而是迭代成本：

| | 改在 ABP | 改在 C++ |
|---|---|---|
| 调一个系数 | 改默认值，立刻看 | 改默认值 + 重编译（Live Coding 还不一定吃） |
| 加一个混合层 | 连一根线 | 加字段 + 重编译 + 蓝图重编译 |
| 出问题定位 | 在 ABP 里断点/看节点 | 在 C++ 里 printf 或调试器 |

所以 C++ 现在只剩三类东西：

1. **物理侧信号**：射线、重力推导、速度积分 —— ABP 里做不了（射线非线程安全）
2. **权威状态**：目标朝向、步态、站姿 —— 必须唯一，否则多端/多系统打架
3. **跨线程安全封装**：`Montage_Play` 在 WT 上调用是未定义行为

---

## 1. Property Access 路径总表

先把 `CharMoveComp` 的名字确认清楚：`ACharacter` 的移动组件子对象名是 **`CharMoveComp`**
（`ACharacter::CharacterMovementComponentName`），Property Access 里选 `Character → CharMoveComp`。

| 需要什么 | 路径 | 类型 | 备注 |
|---|---|---|---|
| 速度（含 Z） | `Character.CharMoveComp.Velocity` | FVector | `UMovementComponent::Velocity`，BlueprintReadWrite |
| 水平速度 | `VSizeXY(Velocity)` | float | 数学节点，不是 Property Access |
| 速度方向 | `Normalize(Velocity)` | FVector | 静止时为零向量，**需要 Select 保护** |
| 是否蹲下 | `Character.bIsCrouched` | bool | `ACharacter` 的 UPROPERTY |
| 大状态 | `CharMoveComp.LocomotionState.Mode` | `EACTLocomotionMode` | Grounded / Jumping / Falling / Landing |
| 是否落地缓冲 | `CharMoveComp.LocomotionState.bLanding` | bool | |
| 到落地时间 | `CharMoveComp.LocomotionState.TimeToLand` | float | 秒 |
| 离地距离 | `CharMoveComp.LocomotionState.DistanceToGround` | float | cm，10Hz 更新 |
| 加速强度 | `CharMoveComp.LocomotionState.AccelerationMagnitude` | float | 已平滑，cm/s² |
| 刹车强度 | `CharMoveComp.LocomotionState.BrakeAmount` | float | 已平滑，0~1 |
| 移动输入 | `CharMoveComp.LocomotionState.MoveInput` | FVector2D | **角色本地**，X=右 Y=前 |
| 步态 | `CharMoveComp.LocomotionState.Gait` | `EACTGait` | Walk/Run/Sprint，**输入意图，不是运动状态**（见 §3.5） |
| 站姿 | `CharMoveComp.LocomotionState.Stance` | `EACTStance` | Stand/Crouch |
| 朝向模式 | `CharMoveComp.LocomotionState.RotationMode` | `EACTRotationMode` | VelocityDirection / LookingDirection |
| 朝向误差 | `CharMoveComp.LocomotionState.YawOffset` | float | 度，目标朝向 − 实际朝向 |
| 摩擦参数 | `CharMoveComp.GroundFriction` / `.BrakingFriction` / `.BrakingFrictionFactor` / `.bUseSeparateBrakingFriction` | float / bool | 喂给停止距离节点 |
| 最大减速度 | `CharMoveComp.GetMaxBrakingDeceleration()` | float | **函数**，Property Access 支持零参原生函数；选不到就改读 `BrakingDecelerationWalking` |

**前提**：`LocomotionState` 已被标成 `BlueprintReadOnly`（已在
`ACTCharacterMovementComponent.h` 里改好）。不标的话编辑器里这条路径根本展开不出来，
而且不会有任何提示 —— 这是第一次接线最容易卡住的地方。

> Property Access 是 GT 拷贝、WT 只读，天然线程安全。每一条路径每帧一次拷贝，
> 数量多了才有成本；单角色几十条完全无所谓。

---

## 2. AnimGraph 拓扑

自下而上（Root 在最上面）：

```
Root
└─ Foot Placement              Graph speed 模式，最后一步：能修正蒙太奇姿势
   └─ Slot "UpperBody"         Phase 6 接攻击/受击（将来用 LayeredBlendPerBone 只覆盖 spine 以上）
      └─ Slot "DefaultSlot"    全身蒙太奇：roll / pivot / mantle
         └─ Stride Warping        Graph
            └─ Orientation Warping Graph
               └─ State Machine
                  ├─ Idle         Sequence Player（静止，随机起始时间打散同步）
                  ├─ Start        Sequence Evaluator + AdvanceTimeByDistanceMatching
                  ├─ Cycle        1D Blend Space（Speed）
                  ├─ Stop         Sequence Evaluator + DistanceMatchToTarget
                  ├─ Jump         Sequence Player
                  ├─ Fall         Sequence Player
                  └─ Land         Sequence Player
```

`Idle` 与 `Cycle` 分开，是为了让 `Idle → Start` 的条件只涉及速度（见 §3.5）；
如果合成一个状态，"是不是刚起步"就得靠额外的 bool 锁存，反而更容易出 bug。

**顺序不能颠倒，颠倒不报错，只会表现为"脚滑/脚悬空"。**

- 两个 Warping 必须在混合**之后**（更靠近 State Machine）：它们修正的是 locomotion 姿势本身。
- Slot 在 Warping **之后**：蒙太奇姿势不该被 Warping 再扭一次。
- Foot Placement 在 Slot **之后**：它要用最终姿势去解 IK，否则脚会踩空。
- **两个 Slot 是串联的**：`DefaultSlot`（内层，全身）先覆盖，`UpperBody`（外层）再盖上半身。
  蒙太奇默认写的是 `DefaultSlot`，只放一个 `UpperBody` 的话 pivot / roll 根本不会播。

`pivot`（跑动中 180° 反打）走 `DefaultSlot`，**不要**为它在状态机里加状态 ——
理由见 `Docs/Turn-180-Design.md` §6.2：那是离散事件，变成状态就需要进出条件，正是抖动来源。

---

## 3. 各节点引脚

### 3.1 Orientation Warping

| 项 | 值 |
|---|---|
| Mode | `Graph` |
| `LocomotionDirection` | `Character.CharMoveComp.Velocity`（**世界空间**，节点内部自己转局部） |
| `LocomotionSpeed` | `VSizeXY(Velocity)` |
| IK Rig | 必填（要 Spine Bones） |

静止时 `Velocity` 是零向量，节点走 fallback、不扭曲 —— 正是想要的行为，不用额外处理。
如果发现腿转向反了：不是接错线，是该骨架的骨骼朝向约定不同，把 IK Rig 里的 Spine Bones 顺序反过来。

> 注意 `LocomotionDirection` 接的是**实际速度**，而 `LocomotionState.YawOffset` 现在表示
> 「**输入意图**方向 − 当前朝向」。两者在转身过程中故意不同：warping 要跟位移（脚踩在哪），
> 意图只用来决定胶囊该往哪转、要不要播 pivot。详见 `Docs/Turn-180-Design.md`。

### 3.2 Stride Warping

| 项 | 值 |
|---|---|
| Mode | `Graph` |
| `LocomotionSpeed` | `VSizeXY(Velocity)`（**不要喂滤波后的值**，Graph 要真实位移速度） |
| `StrideDirection` | 可不接（默认 forward） |
| IK Rig | 必填（要 Pelvis + 双脚） |

Graph 模式下 `StrideScale` 引脚**被完全忽略**（`AnimNode_StrideWarping.cpp:203-210` 只有 Graph 分支读 `LocomotionSpeed`）。
任何 clip 缺 root motion，该帧 delta 为 0 → 节点静默不缩放 → 偶发滑步。所以**同一混合链里不要混无 RM 的 clip**。

### 3.3 Foot Placement

| 项 | 值 |
|---|---|
| `PlantSpeedMode` | `Graph` |
| `bEnabled` / 混合权重 | 接 `LocomotionState.Mode == Grounded`（空中开着会去贴空气） |
| `Trace Settings → Start Offset` | 按骨架实测，默认 -75 是给标准身高的 |
| IK Rig | 必填（Pelvis + 双脚 + 脚趾） |

`Start Offset` 偏了的表现是**脚悬空或穿地，且不报任何错**。四套骨架身高不同，这个值必须一个一个调。

### 3.4 Cycle 的 Blend Space

- **1D**，只有 Speed 轴
- X 轴接 `VSizeXY(CharMoveComp.Velocity)`，**不要**接滤波后的值

为什么不加方向轴：转身时速度方向会先冲 90° 再回落，2D BS 会跟着播一遍横移动画，
和 Orientation Warping 叠加成双重转向。**BS 只管"多快"，Orientation Warping 管"朝哪"。**

#### 采样点该放多少（Graph 模式下这条容易搞错）

**采样点的值 = 该 clip 自己的 root motion 速度**，不是"角色想跑多快"。

理由：Graph 模式的 Stride Warping 用 `LocomotionSpeed / CachedRootMotionDeltaSpeed` 算缩放，
分母是**混合后姿态的 RM 速度**。如果采样点落在一个和 clip 实际速度差很远的位置，
分母和分子长期不匹配，节点就得一直大幅拉扯 —— 表现为脚要么迈得太夸张，要么被压扁、步频不对。

测量方法（任选其一）：

1. 烘焙完 `Distance` 曲线后，`曲线末值 / clip 时长` ≈ 平均 RM 速度
2. Animation Sequence 编辑器 → `Root Motion` 面板看总位移，除以时长
3. 直接看脚步：踩地那一刻脚相对地面的位移速度

参考量级（PC_01 这套步速）：`Walk ≈ 150`、`Jog ≈ 500`。**以实测为准，不要照抄。**

轴的 `Max` 至少覆盖 Sprint 的 `MaxSpeed`。轴到头了会截断（永远播最右那个采样点的姿势），
剩下的差额全压给 Stride Warping，超过它的拉伸上限就是滑步。

#### Idle 放不放进去

**要放，而且 `0` 采样点必须是 Idle 姿势。**

之前这里写的是"0 点留给正在停但还没停住的过渡"，那是错的：`Stop` 是独立状态，有自己的
SequenceEvaluator + `DistanceMatchToTarget`，根本不经过 BS。而 `Cycle` 里速度仍可能掉到 0
（顶着墙推、被击退、胶囊体卡在缝里），BS 采样点落在最左端，必须是一个合理的静态姿势。

另外一个理由：`Stop` 播到最后就是 Idle 姿势，`0` 点放 Idle 才能让 `Stop → Idle` 无跳变。

把 Idle 塞进 BS 会让"停住"这件事由混合权重决定，它和距离驱动的 Stop 状态会互相打架：
两套逻辑都在决定"现在播到哪"，结果就是停止动画播到一半被 BS 拽回 Idle。

#### 一个删掉 `AnimSpeed` 后才出现的边界情况

原来 BS 的轴接的是带惯性的 `AnimSpeed`，速度瞬变会被平滑掉。现在接真实速度，
下面这种情况会看到姿势突变：

- 撞墙、被击退、位移被强制归零 → 速度在一帧内掉到 0 → BS 瞬间跳到最左端

正常急停不会出问题（Stop 状态接住了），但撞墙没有专门的动画状态。
好在 `0` 采样点现在是 Idle 姿势，跳过去至少落在一个合法姿势上，不会出现"半停不停"的怪姿势。
**先用着，验收时留意**；如果确实难看，在 ABP 里给这根线加一个 `FInterpTo`（一根线，20 秒的事），
或者给撞墙单独切受击状态 —— 仍然不需要回 C++。

### 3.5 State Machine 转换条件

状态集合（比 §2 的拓扑多了独立的 `Idle`，理由见下）：

```
Idle ──速度起来──> Start ──距离用完──> Cycle ──松输入──> Stop ──停稳──> Idle
 └────────────── Mode == Jumping ──────────────> Jump → Fall → Land → Idle
```

| 转换 | 条件 | 数据来源 |
|---|---|---|
| `Idle → Start` | `Speed > 15` | `VSizeXY(Velocity)` |
| `Start → Cycle` | `DistanceTraveled >= StartClipDistance` | ABP 变量 + clip 总距离常量 |
| `Start → Stop` | `MoveInput.Size() < 0.1 && Speed < 20` | 起步途中松手 |
| `Cycle → Stop` | `MoveInput.Size() < 0.2 && Speed > 20` | 松输入但还在滑 |
| `Stop → Idle` | `Speed < 5` | |
| `Idle → Jump` | `Mode == Jumping` | `LocomotionState.Mode` |
| `Jump → Fall` | `Mode == Falling` | |
| `Fall → Land` | `bLanding` | |
| `Land → Idle` | `!bLanding` | |

`Mode` 的物理判断（含 `Landing` 那 0.25 秒缓冲）在 C++ 里做完，ABP 不重复推导。

#### 为什么 `Idle → Start` 用速度，不用 `MoveInput`

`AdvanceTimeByDistanceMatching` 是靠**累计位移**推进 clip 的。速度还是 0 时位移不增长，
clip 会冻在第 0 帧 —— 真触发了也是卡住不动。用速度做条件天然自洽：没有位移就永远不进 Start。

阈值取 15 cm/s 是为了滤掉静止时 `Velocity` 的浮点残差，**不要设成 0**。

#### 为什么 `Cycle → Stop` 要带 `MoveInput`，不能只看 `BrakeAmount`

Run→Walk 降档时 `SmoothedMaxSpeed` 用 0.2 秒插值过去，速度会从 500 掉到 150，
`BrakeAmount` 一样会冲起来。只看 `BrakeAmount`，会在玩家**仍按着前进键**的时候插一段停止动画。
带上"松输入"这个条件，语义才和停止动画一致。

#### 为什么不能用 `EACTGait` 做转移条件

**`Gait` 是输入意图，不是运动状态。** 它只由输入设置，完全不看速度
（`ACTCharacterBase.cpp:207-220`）：

```cpp
SetGait(ActionValue.Get<bool>() ? EACTGait::Sprint : (bDesiredWalk ? EACTGait::Walk : EACTGait::Run));
```

三个反例：

| 情形 | `Gait` 的值 | 实际状态 | 用 Gait 判的结果 |
|---|---|---|---|
| 站着按住 Shift | `Sprint` | 完全静止 | 误判"起步了"，原地播起步动画 |
| Idle（Run 档）推摇杆 | 全程 `Run` | 开始移动 | `Gait == Run` 恒真，**没有边沿**，进不去 Start |
| 跑动中按 Shift 升档 | `Run → Sprint` | 已经在跑 | 误判"起步"，速度爬升途中插一段起步动画 |

第二行是致命的：`Gait` 在「静止 → 起步 → 匀速」整个过程中**一个值都不变**，
它根本携带不了"运动开始了"这个信息。换成 `Gait != Run` 也一样 —— 那变成"按下 Shift 就播起步"。

#### `Gait` 的正确用途：锁存后决定播哪一段

`Gait` 决定的是**播哪个 clip**，不是**什么时候播**。而且必须在**进入状态那一刻锁存**：

```
State: Start
  On State Entry  →  LatchedGait = CharMoveComp.LocomotionState.Gait
  Sequence Evaluator 的 clip ← Select(LatchedGait, StartClipTable)
  On State Exit   →  DistanceTraveled = 0
```

用 State Result 节点的 **On State Entry / On State Exit** 函数做
（`AnimGraphNode_StateResult.h:49-71`，UE5 的状态函数，走 `Prototype_ThreadSafeAnimUpdateCall`，
线程安全）。

**不要每帧读 `Gait` 直接切 clip**，否则：

- 起步途中按 Shift → `Gait` 变 Sprint → SequenceEvaluator 换 clip
- 新 clip 的 `Distance` 曲线和旧的不同 → 已累计的 `DistanceTraveled` 对应到新曲线上的位置完全不同
  → 时间被重算 → **肉眼可见的姿势跳变**
- `Stop` 同理：松手瞬间改步态会让停止动画跳到另一个 clip 的中间

`Stop` 复用同一个 `LatchedGait` 变量即可：进入 Stop 时锁存的是上一次加速用的档位，
正好对应"用什么姿势停下来"。

#### Gait → clip 映射表（不能枚举对枚举）

| `EACTGait` | Start clip | Stop clip |
|---|---|---|
| `Walk` | `PC_01_Idle_to_Walk_Fwd` | `PC_01_Walk_Fwd_to_Idle` |
| `Run` | `PC_01_Idle_to_Jog_Fwd` | `PC_01_Jog_Fwd_to_Idle` |
| `Sprint` | **无专用**（候选：`A_PC_01_Sprint_Acceleration`） | **无专用**（候选：`A_PC_01_Stop_Left/Right`） |

`EACTGait` 有三个值，`PC_01_*` 那套资产只有两套起停。**直接 `Switch on EACTGait` 三分支接 clip，
Sprint 那两支没有资产可接**。两种处理：

1. Sprint 暂时指向 Jog 的 clip（速度差靠 `DistanceMatching` + `StrideWarping` 吸收一部分，
   但 900cm/s 用 Jog 起点会明显"急"—— 这是资产缺口，不是代码问题）
2. 打开 `A_PC_01_Sprint_Acceleration` 确认它是不是"静止→冲刺"的过渡段（也可能只是一段可循环的加速，
   那就不能当 Start clip 用），`A_PC_01_Stop_Left/Right` 同理

用 `Select` 做两层映射，**不要用 Switch 直接连资产**：以后补了 Sprint 的 clip 只改一个分支，
不用动状态机结构。

---

## 4. Distance Matching

### 4.1 先烘焙 Distance 曲线

选中动画 → 右键 **Add Animation Modifier** → `Distance Curve Modifier`
（`AnimationLocomotionLibrary` 自带，源码 `Source/Editor/Public/DistanceCurveModifier.h`）。

| 参数 | 起步 clip | 停止 clip |
|---|---|---|
| `CurveName` | `Distance` | `Distance` |
| `bStopAtEnd` | false | **true** |
| `Axis` | `XY` | `XY` |

曲线语义（头文件原话）：
> A negative value indicates distance remaining to a stop or pivot point.
> A positive value indicates distance traveled from a start point or from the beginning of the clip.

所以一份曲线天然两用：正段给 `AdvanceTimeByDistanceMatching`，负段给 `DistanceMatchToTarget`。
`bStopAtEnd=false` 时它会用 120Hz 扫描找速度最低点当停止点；判断不准就手动勾上。

> Modifier 是**永久改写资产**。批量应用前先复制一份做测试。

### 4.2 Start：`Advance Time By Distance Matching`

```
Sequence Evaluator (start clip)
  → AdvanceTimeByDistanceMatching(UpdateContext, SequenceEvaluator, DistanceTraveled, "Distance")
```

`DistanceTraveled` 需要自己在 ABP 里积分（没有现成节点）：

```
[State: Start] 每帧:
  DistanceTraveled = DistanceTraveled + (VSizeXY(Velocity) * DeltaTime)
[离开 Start] DistanceTraveled = 0
```

用 `Blueprint Thread Safe Update Animation` 里一个 float 变量（`AnimInstance.h:580`，会传入 `DeltaTime`）。

**不要放在 `Blueprint Update Animation`**：那是 GT，而 `DistanceTraveled` 的消费方是 State Machine
（URO 下在 WT 求值）。GT 写、WT 读是数据竞争 —— 编译不报错，只表现为偶发的进度跳一下。
`LatchedGait`（§3.5）同理。

`DeltaTime` 用函数入参，不要用 `GetWorld()->DeltaTimeSeconds`（URO 下两者不一致）。

另外注意：`DistanceTraveled` 必须在 **State Exit** 清零，而不是靠 `Speed < threshold` 判 ——
否则下一次起步会带着上一轮残留的距离，起步 clip 直接从中间开始。

### 4.3 Stop：`Distance Match To Target`

```
Sequence Evaluator (stop clip)
  → DistanceMatchToTarget(SequenceEvaluator, DistanceToTarget, "Distance")

DistanceToTarget = Vector Length of
  PredictGroundMovementStopLocation(
      Velocity          ← Character.CharMoveComp.Velocity
      bUseSeparateBrakingFriction ← CharMoveComp.bUseSeparateBrakingFriction
      BrakingFriction   ← CharMoveComp.BrakingFriction
      GroundFriction    ← CharMoveComp.GroundFriction
      BrakingFrictionFactor ← CharMoveComp.BrakingFrictionFactor
      BrakingDecelerationWalking ← CharMoveComp.BrakingDecelerationWalking
  )
```

这个节点就是之前 C++ 里 `BrakingDistance` 干的事，而且更完整（含摩擦项）。
它 `BlueprintThreadSafe`，可以放心放在 WT 上。

### 4.4 Pivot：`Predict Ground Movement Pivot Location`

留给 Phase 5 的复杂转身：
`PredictGroundMovementPivotLocation(Acceleration, Velocity, GroundFriction)` 告诉你
急转输入下速度方向真正翻过来的点在哪，正好是转身动画该"蹬地"的位置。

`Acceleration` 是 `UCharacterMovementComponent` 的 **protected 属性**，Property Access 选不到。
两个办法（按优先级）：

1. 用 `CharMoveComp.LocomotionState.AccelerationMagnitude` 的方向近似 → 需要额外暴露方向，暂时别用
2. **首选**：在 C++ 里加一个 `BlueprintReadOnly FVector LastAcceleration`（一行），ABP 读它

第 2 条是"节点拿不到的数据"的典型例子 —— 属于该留在 C++ 的那一类。

---

## 5. Lean（ABP 数学，不需要 C++）

在 Event Graph 的 `Blueprint Update Animation` 里算，输出两个 float 变量，
AnimGraph 里接 `Modify Bone`（脊柱 pitch / roll）或 Control Rig。

### 5.1 前后倾

```
Sign   = Select(BrakeAmount > 0.5, -1.0, Sign(MoveInput.Y))     // 刹车时后仰
Alpha  = Clamp(AccelerationMagnitude / 1500.0, 0.0, 1.0)
Lean   = Sign * Alpha * 12.0
```

### 5.2 转向侧倾

不要用"上一帧 yaw 差分"（噪声大、首帧脏、URO 会跳）。用稳态关系更干净：

```
// 朝向误差按 exp 收敛，稳态角速度 ω ≈ YawOffset × k
// 向心加速度 a = v × ω  ⇒  侧倾 ∝ Speed × YawOffset
Alpha = Clamp((Speed * YawOffset) / 30000.0, -1.0, 1.0)
Lean  = Alpha * 10.0
```

`30000 ≈ 500(cm/s) × 60(deg)` 是"满速、误差 60°"的参考量，实际按手感调。
这比差分版平滑得多 —— 也是把计算交给 ABP 之后自然出现的简化，因为直接在 C++ 里写就
很容易顺手用差分。

---

## 6. 对照表：原来 C++ 的字段都去哪了

| 原 `UACTAnimInstance` 字段 | 现在 |
|---|---|
| `Speed` | `VSizeXY(CharMoveComp.Velocity)` |
| `AnimSpeed` | **删除**。1D BS 直接用真实 Speed，瞬态由 Stride Warping 吸收 |
| `Direction` | **删除**。1D BS 没有方向轴 |
| `OrientationAngle` | Graph 模式的 `LocomotionDirection` |
| `StrideScale` | Graph 模式的 `LocomotionSpeed` |
| `VelocityDirection` | `Normalize(CharMoveComp.Velocity)` |
| `BrakingDistance` | `PredictGroundMovementStopLocation` → `Vector Length` |
| `StopLocationOffset` | 同一节点的 FVector 输出 |
| `LocomotionMode` | `LocomotionState.Mode` |
| `bCrouched` | `Character.bIsCrouched` |
| `AccelerationLean` | §5.1 的 ABP 数学 |
| `TurnLean` | §5.2 的 ABP 数学 |
| `LandPredictionAlpha` | `Clamp(1 - TimeToLand / 0.35, 0, 1)`，`Mode == Falling` 时有效 |

`UACTAnimInstance` 现在只剩蒙太奇队列（约 130 行）。它不覆写
`NativeThreadSafeUpdateAnimation`，因为已经没有 WT 逻辑了。

---

## 7. 什么时候该转回 C++

按这个顺序判断，别凭感觉：

1. **WT 预算**：`stat anim` 里动画更新超过 0.3ms/帧，且 profiler 显示时间在 Property Access 或数学节点上
2. **节点数量**：同一段逻辑在 3 个以上的地方各连一遍（改一处漏两处）
3. **需要跨帧状态**：积分、滤波、状态机 —— ABP 变量能做，但没法断点、没法单测，出错极难查
4. **需要 UObject 访问**：WT 上读 UObject 属性是数据竞争，只能回 GT 或回 C++

回退成本很低，因为分工是清晰的一条线：把计算放回 `NativeThreadSafeUpdateAnimation`，
ABP 里把对应的 Property Access / 数学节点换成读一个 `BlueprintReadOnly` 变量。

**唯一要注意**：一旦回 C++，就必须恢复 GT 快照（`NativeUpdateAnimation` 里拷一份），
因为 WT 读 UObject 属性不安全。这也是当初 `FActLocomotionState` 存在的理由 ——
现在它依然在，只是消费方从"动画实例"变成了"ABP 的 Property Access"。

---

## 8. 验收清单

- [ ] ABP 里没有 cast、没有 `GetPlayerCharacter`
- [ ] Property Access 路径全部能展开（选不到的属性就是没标 BlueprintReadOnly）
- [ ] State Machine 只有 5~6 个状态
- [ ] `0 / 150 / 300 / 600` 四个速度走直线，脚不打滑
- [ ] 加速起步：脚从第一步就踩实，不是"原地蹭两下再出去"
- [ ] 急停：停止动画结束时角色刚好停住，没有"滑一下"
- [ ] 90° 急转：脚不交叉、不悬空
- [ ] `stat anim` 里 Anim Instance 时间 < 0.2ms
