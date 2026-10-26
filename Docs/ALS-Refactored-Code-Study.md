# ALS-Refactored C++ 源码分析笔记

> 插件版本：ALS-Refactored 4.18（`EngineVersion: 5.8`）  
> 源码规模：4 个模块 / 约 18,000 行 C++（不含内容资源）  
> 本文所有代码位置均为 `文件:行号`（相对于 `Plugins/ALS-Refactored/Source/`）

---

## 0. 一句话概括

ALS 的本质不是"一套动画资源"，而是**三层严格分离的状态机**：

1. **角色层**（`AAlsCharacter`）—— 唯一的状态所有者与旋转权威，把"玩家意图"解析成"实际状态"；
2. **移动层**（`UAlsCharacterMovementComponent`）—— 把速度/加速度/摩擦交给曲线，把"会影响预测的状态"塞进 `FSavedMove`；
3. **动画层**（`UAlsAnimationInstance`）—— 把角色状态 + 动画曲线折算成"表现参数"，并全部搬到 Worker 线程计算。

真正值得学的，是这三层之间**谁拥有什么、什么时候读、通过什么通道同步**。

---

## 1. 模块与依赖

| 模块          | 类型           | 职责                                                                     |
| ----------- | ------------ | ---------------------------------------------------------------------- |
| `ALS`       | Runtime      | 核心：角色 / 移动 / 动画实例 / 动画节点 / RigUnit / Notify / 设置资产                     |
| `ALSCamera` | Runtime      | 相机以 `USkeletalMeshComponent` 子类实现，由相机 ABP 驱动（socket 定位 + 曲线调参）         |
| `ALSExtras` | Runtime      | 示例角色 `AAlsCharacterExample`、AI Controller、Level                        |
| `ALSEditor` | UncookedOnly | Skeleton 一键配置、Animation Modifier（生成曲线/FootSyncMarker）、AnimGraphNode 包装 |

外部插件依赖（`ALS.uplugin:41-77`）：ACL、AnimationModifierLibrary、ControlRig、EnhancedInput、GameplayTagsEditor、Metasound、Niagara、PropertyAccessNode、EngineCameras。

关键结论：**GameplayTag 替代枚举 + ControlRig 替代 AnimNode IK + PropertyAccess 替代直接读取**，这三者是 ALS 现代化改造的骨架。

---

## 2. 五个核心设计范式

### 2.1 Desired / Actual 双状态

```cpp
// AlsCharacter.h:36-72
bDesiredAiming / DesiredRotationMode / DesiredStance / DesiredGait   // 意图（玩家想做什么）
LocomotionMode / RotationMode / Stance / Gait / LocomotionAction     // 实际（规则允许什么）
```

- **Desired** 由输入直接写，`COND_SkipOwner` + Push Model 复制给其他端；
- **Actual** 每帧由 `RefreshGait()` / `RefreshRotationMode()` 重新推导，**不复制**。

好处：多端不会因为"中间状态"不同步而抖动，只同步意图，结果各端独立收敛。

派生逻辑举例（`AlsCharacter.cpp:675-758`）：冲刺时是否允许瞄准、第一人称与第三人称分支、空中是否保持瞄准……全部收敛在一个纯函数里。

### 2.2 单一旋转权威

```cpp
// AlsCharacter.cpp:1504
void AAlsCharacter::FaceRotation(const FRotator Rotation, const float DeltaTime)
{
    // 故意留空：忽略一切外部旋转来源
}
```

同时 `RotationRate = ZeroRotator`、`bOrientRotationToMovement = false`、`bUseControllerDesiredRotation = false`（`AlsCharacterMovementComponent.cpp:174-178`）。

**所有旋转只能由 `RefreshGroundedRotation()` / `RefreshInAirRotation()` 产生**，通过 4 个出口：

| 函数                                 | 语义                         |
| ---------------------------------- | -------------------------- |
| `SetTargetYawAngle()` `:1927`      | 直接对齐目标                     |
| `SetRotationSmooth()` `:1887`      | 单段阻尼                       |
| `SetRotationExtraSmooth()` `:1898` | 双段：先限速平滑目标角，再阻尼追踪（防"目标瞬移") |
| `SetRotationInstant()` `:1910`     | 瞬转（翻滚起手 / 落地净角）            |

这是接入自定义战斗动作时**最容易踩坑的地方**：任何直接 `SetActorRotation` 的行为都会被下一帧的 ALS 逻辑覆盖，必须走上面 4 个出口或设置 `LocomotionAction`。

### 2.3 GameplayTag 作为状态维度

`AlsGameplayTags.h` 定义 7 组原生标签：ViewMode / LocomotionMode / RotationMode / Stance / Gait / OverlayMode / LocomotionAction。

- 每个 tag 都是**闭合枚举**语义（`==` 比较、`IsValid()` 判空）；
- `OverlayMode` 用 tag 表达"持枪/持弓/受伤/被绑"等，动画侧用 `Blend Poses by Gameplay Tag` 节点消费（`Nodes/AlsAnimNode_GameplayTagsBlend.cpp:5`，`GetTags().Find(ActiveTag) + 1` —— 索引 0 是默认姿势）；
- 新增状态**无需改 C++**。

### 2.4 `LocomotionAction` 作为动作互斥锁

```cpp
// AlsCharacter.h:72
FGameplayTag LocomotionAction;   // Rolling / Mantling / Ragdolling / GettingUp
```

它同时充当"当前是否在播放占位动作"的全局判据，被以下位置检查：

- `ApplyDesiredStance()` `:801` —— 动作中不蹲/不站；
- `RefreshGroundedRotation()` `:1516` —— 动作中不做地面旋转；
- `RefreshInAirRotation()` `:1822`；
- `IsMantlingAllowedToStart()` `:162`、`IsRagdollingAllowedToStart()` `:715`；
- 动画实例 `RefreshLocomotionOnGameThread()` 中同步后触发 `ResetGroundedEntryMode()`。

> ⚠️ 这是**单值**语义。ACT 游戏要求"攻击中可被位移、可被打断、可接翻滚"，单值不够表达层级，详见第 9 节。

### 2.5 数据与表现分离

角色层只产出**数值**（速度、角度、速度朝向、目标角），动画层只做**映射**（曲线求值、混合权重、Foot Lock 位置）。中间媒介是 20 多个 `State/*.h` 结构体（`FAlsFeetState`、`FAlsLayeringState`、`FAlsPoseState`…）以及 `FAlsControlRigInput`。

`FAlsControlRigInput`（`AlsAnimationInstance.cpp:235-257`）是 C++ → ControlRig 的唯一通道，把 Foot IK 目标、Spine 扭转、VelocityBlend、PelvisOffset 打包成纯数据结构。

---

## 3. 角色层：`AAlsCharacter`

### 3.1 生命周期钩子

| 阶段                         | 做的事                                                                                                      | 位置         |
| -------------------------- | -------------------------------------------------------------------------------------------------------- | ---------- |
| 构造                         | `SetDefaultSubobjectClass` 换掉 CMC；`bUseControllerRotation* = false`；Mesh 偏移 `-92 / -90°`                 | `:22-59`   |
| `PreRegisterAllComponents` | 提前把 RotationMode/Stance/Gait/ViewState 初始化好，供动画实例与相机在初始化期读取                                              | `:92-118`  |
| `PostInitializeComponents` | `GetMesh()->AddTickPrerequisiteActor(this)`（Mesh 必须晚于角色 Tick）；绑定 `OnPhysicsRotation`；下发 MovementSettings | `:120-135` |
| `BeginPlay`                | 注册 Teleport 检测 lambda、网络平滑开关、`ApplyDesiredStance()`                                                      | `:137-181` |

`AddTickPrerequisiteActor` 是整个插件的时间基准：**角色先算完状态，Mesh/ABP 才读**。

### 3.2 Tick 管线（`AlsCharacter.cpp:266-301`）

```
RefreshMovementBase()       // 脚下平台变换 + DeltaRotation
RefreshMeshProperties()     // 动画 Tick 策略 / 绝对旋转 / bPendingUpdate
RefreshInput()              // 从加速度反推输入方向（唯一让其他端也能拿到输入的地方）
RefreshLocomotionEarly()    // 旋转平台继承、上一帧目标角继承
RefreshView()               // 视角复制 + 网络平滑 + YawSpeed
RefreshLocomotion()         // 速度、速度朝向、理想速度朝向（PrePenetration 速度）
RefreshGait()               // MaxAllowedGait → CMC，ActualGait
RefreshRotationMode()       // 意图 → 实际旋转模式
RefreshGroundedRotation()   // 地面旋转（4 种模式 × 移动/静止 = 8 分支）
RefreshInAirRotation()
AutoStartMantling() / RefreshMantling() / RefreshRagdolling() / RefreshRolling()
Super::Tick()
RefreshLocomotionLate()     // 收尾：目标角对齐、AimingLimit 复位
```

值得注意的两个"Early/Late"：把**依赖上一帧结果的量**放在最前，把**依赖本帧结果的量**放在最后，中间是纯计算，顺序不可随意调整。

### 3.3 旋转系统细节

**地面移动（ViewDirection / Aiming）**：

```cpp
// :1649-1651
TargetYawAngle = ViewState.Rotation.Yaw + Mesh->GetAnimInstance()->GetCurveValue(RotationYawOffsetCurve);
```

即：**角色朝向 = 摄像机朝向 + 动画曲线给的偏移**。动画师通过曲线控制"跑动时身体偏转多少"，C++ 只负责读取 —— 表现力交给动画，逻辑保持简单。

**瞄准约束**（`:1712-1781`）：`ConstrainAimingRotation` 把"角色与视角的夹角"限制在 `AimingYawAngleLimit`（默认 70°），并对每帧做 `bResetAimingLimit` 状态机：

- 当帧没触发约束 → 下一帧把上限复位到 180°（允许慢慢追）；
- 触发约束 → 上限收缩到当前夹角（防止下身跟不上）。

配合 `bApplySecondaryConstraint` 做二次收敛，避免"卡在边界上抖动"。

**旋转速率**来自曲线：`CalculateGroundedMovingRotationInterpolationHalfLife()` `:1783` 用 `RotationInterpolationSpeedCurve(GaitAmount)` 得到 HalfLife，再按视角转速缩短（相机转得快 → 角色跟得快）。

### 3.4 视角与网络平滑

复制的是 `ReplicatedViewRotation`（`FRotator`，Unreliable Server RPC，仅在不复制移动时手动发送，`:1247-1253`）。

接收端用**仿 CMC 的平滑算法**（`:1269-1328`）：

```
ClientTime += DeltaTime
Alpha = 1 - (ServerTime - ClientTime) / Duration
FinalRotation = Lerp(Initial, Target, Alpha)
```

`Duration` 由 `ServerLastTransformUpdateTimeStamp` 差值算出（`CorrectViewNetworkSmoothing()` `:1163`）。

**站位在旋转平台上**时，所有角度先转到"平台空间"再插值（`:1294-1309`），否则会随平台一起抖。这是全套代码里最容易被忽略、也最难自研的部分。

---

## 4. 移动层：`UAlsCharacterMovementComponent`

### 4.1 扩展网络预测（最精彩的 40 行）

```cpp
// AlsCharacterMovementComponent.h:9-71
FAlsCharacterNetworkMoveData : FCharacterNetworkMoveData
FAlsSavedMove               : FSavedMove_Character
```

- 把 `RotationMode / Stance / MaxAllowedGait` 塞进每一次 move 的序列化（`:32-34`，用 `NetSerializeOptionalValue` 带默认值，省带宽）；
- `CanCombineWith()` 里**只要这三个 tag 不同就不合并 move**（`:69-77`）；
- `PrepMoveFor()` 在服务器重放时恢复这三个值并 `RefreshGaitSettings()`（`:102-115`）。

解决的问题：**最大速度是随 stance/gait 变化的**。如果 move 里不带这个信息，服务器重放时用错速度 → 位置修正 → 橡皮筋。

> 这条经验对 ACT 直接适用：攻击时若改 `MaxWalkSpeed`、`GroundFriction`、`BrakingDeceleration`，都必须进 move data，否则联机必然拉扯。

### 4.2 `PhysWalking` 的整体拷贝（`:333-595`）

ALS 直接把引擎的 `PhysWalking` 抄了一份，只改两处：

1. **允许在可行走地面上也做穿透修正**（`:528-540`）；
2. **在执行穿透修正前记录速度**（`:568-573`）：

```cpp
PrePenetrationAdjustmentVelocity = MoveVelocity;
bPrePenetrationAdjustmentVelocityValid = true;
```

这个速度后续被 `TryConsumePrePenetrationAdjustmentVelocity()` 消费，用于"撞墙时角色仍朝**想去的方向**转身"（`bRotateTowardsDesiredVelocityInVelocityDirectionRotationMode`）。

代价：**升级引擎版本必须手动 merge**。代码里有 TODO 提示（`:339-340`）。

### 4.3 速度/摩擦曲线化

```cpp
// :1008-1017
MaxAccelerationWalking = Curves[0].Eval(GaitAmount);
BrakingDecelerationWalking = Curves[1].Eval(GaitAmount);
GroundFriction = Curves[2].Eval(GaitAmount);
```

一个 `UCurveVector` 的三条通道 = 加速 / 减速 / 摩擦。`GaitAmount` 是 0~3 的连续值（静止→走→跑→冲刺），由速度映射得到（`:971-984`）。

配合"方向相关速度"（`:939-966`）：前后速度按视角夹角插值，`VelocityAngleToSpeedInterpolationRange` 默认 `[100°,125°]` 才开始过渡到后退速度。

### 4.4 输入阻断

`bInputBlocked` 同时影响 `ConsumeInputVector()` `:202`、`ApplyRequestedMove()` `:271`、`ShouldPerformAirControlForPathFollowing()` `:240`。这是"动作期间禁止移动"的统一开关，比在每个动作里判断更干净。

---

## 5. 动作层：三种截然不同的实现范式

`AlsCharacter_Actions.cpp` 里三个动作，恰好代表三种技术路线 —— **这是整个插件最有教学价值的部分**。

|      | Roll 翻滚                                                   | Mantle 攀爬                                     | Ragdoll 布娃娃                 |
| ---- | --------------------------------------------------------- | --------------------------------------------- | --------------------------- |
| 位移来源 | 蒙太奇 Root Motion（引擎托管）                                     | **自定义 RootMotionSource**（程序化生成）               | 物理模拟（脱离胶囊）                  |
| 移动模式 | `MOVE_Walking`（不变）                                        | `MOVE_Custom` + `SetMovementModeLocked(true)` | `MOVE_None`                 |
| 旋转控制 | `SetRotationInstant` + `PhysicsRotation` 里二次插值 `:116-139` | RMS 内部 twist 对齐重力                             | 由物理决定，结束后按骨盆朝向重建            |
| 同步方式 | `Multicast` / `Server RPC` 双路                             | 同左，参数 `FAlsMantlingParameters` 整体复制           | 目标位置单独 Unreliable 复制 + 拉力修正 |
| 结束条件 | 蒙太奇结束                                                     | RMS 到期 / 目标被销毁                                | 玩家输入 / 落地                   |
| 关键难点 | 服务器上必须持续 Tick Pose（`:337-346`）                            | 蒙太奇时间与 RMS 时间对齐                               | 胶囊与 Mesh 的位置重建              |

### 5.1 Roll 的"预测 + 广播"模式

```cpp
// :61-71
if (GetLocalRole() >= ROLE_Authority)      MulticastStartRolling(...);
else {
    GetCharacterMovement()->FlushServerMoves();   // 关键：先结算已发生的移动
    StartRollingImplementation(...);               // 本地立即播放
    ServerStartRolling(...);                       // 再上报
}
```

`FlushServerMoves()` 保证服务器收到的"滚动起点"与客户端看到的起点一致，否则翻滚会从错的位置开始。

### 5.2 Mantle：自定义 RootMotionSource（`:37-127`）

不依赖蒙太奇自带 root motion，而是：

1. 三条 trace 确定目标（前向胶囊扫描 → 向下球扫 → 目标位置胶囊重叠 → 起点高度重叠）；
2. 二分查找蒙太奇里"垂直高度 = MantlingHeight"的时间点作为起始时间（`:617-639`）；
3. 用起始/结束 root transform 反解出 actor 的 Start/Target 变换，写进 RMS；
4. `PrepareRootMotion()` 里每帧：`Montage_SetPosition()` 同步动画时间 → `MotionWarpingTimeRange` 控制 warp 权重 → 用当前 root transform 反推目标 actor 变换 → 输出 delta。

**为什么这样做**：RMS 会参与移动预测与网络校正，天然支持"被推、被打断、站在移动平台上"，而蒙太奇 root motion 不行。RMS 还实现了 `NetSerialize()` `:129`，网络复制自带。

### 5.3 Ragdoll：物理态与胶囊态的双向重建

启动（`:757-842`）：`Montage_Stop` → `NetworkSmoothingMode = Disabled` → Mesh **Detach** → 胶囊关碰撞 → Mesh 开物理 → 读取骨盆速度作为初速 → `MOVE_None`。

维持（`:866-959`）：每帧读骨盆位置；本地控制端把这位置复制给其他端（`RagdollTargetLocation`），其他端用**弹簧拉力**把布娃娃拉向该位置（`:907-939`，超过 300 速度改拉 Spine03 而非 Pelvis）。

结束（`:1062-1163`）：快照当前 pose → 球扫找地 → 依据骨盆 Roll 决定面朝上/下 → 重建 actor 变换 → Mesh 重新 Attach → 用快照覆盖骨盆局部变换（视觉不跳）→ 播起身蒙太奇。

---

## 6. 动画层：`UAlsAnimationInstance`

### 6.1 两段式线程模型（本插件最核心的性能设计）

```
NativeUpdateAnimation()              [Game Thread]
  ├─ 读 Character 所有 Get*()、Socket、Capsule、Mesh 变换
  ├─ 刷新 MovementBase / View / Locomotion / InAir / Feet(目标) / Ragdoll
  └─ 写只读状态（WT 只读，不再改）

NativeThreadSafeUpdateAnimation()    [Worker Thread]
  ├─ RefreshLayering()   ← 读动画曲线（曲线在这时已求值完成）
  ├─ RefreshPose()
  ├─ RefreshView() → RefreshSpine()
  ├─ RefreshFeet() → Foot Lock
  └─ RefreshTransitions()

NativePostUpdateAnimation()           [Game Thread]
  ├─ PlayQueuedTransitionAnimation()   ← 蒙太奇不能在 WT 播放，必须排队回 GT
  ├─ PlayQueuedTurnInPlaceAnimation()
  └─ bPendingUpdate = !HasActorRegisteredAllComponents()
```


`NativePostUpdateAnimation` 不是原生虚函数，作者用自定义 Proxy 的 `PostUpdate()` 打了一个补丁（`AlsAnimationInstanceProxy.cpp:9-21`），注释写得很直白："Epic 不让我们 override"。

**线程纪律**（`AlsLinkedAnimationInstance.h:33-36` 的注释是最好的说明）：只有 `NativeUpdateAnimation()` 里写入的变量才能被 WT 安全读取；其他函数的写入点在 GT/WT 之间没有同步保证。

### 6.2 曲线读取的"黑科技"

```cpp
// AlsAnimationInstance.cpp:26-27
ALS_DEFINE_PRIVATE_MEMBER_ACCESSOR(AlsGetAnimationCurvesAccessor, &FAnimInstanceProxy::GetAnimationCurves,
                                   const TMap<FName, float>& (FAnimInstanceProxy::*)(EAnimCurveType) const)
```

`GetAnimationCurves` 在 `FAnimInstanceProxy` 里是 protected，直接调用会编译失败。作者用**模板静态成员指针 + 显式实例化**（`AlsPrivateMemberAccessor.h`）绕过了访问控制 —— 因为 `UAnimInstance::GetCurveValue()` 在 WT 上不安全/未求值完成，必须直接读曲线 Map 的 AttributeCurve。

这是"想上 Worker Thread 但引擎 API 不给"的典型工程妥协，值得记住手法（但升级引擎时要验证符号签名）。

### 6.3 状态结构体即"ABP 的输入契约"

`State/` 下 20 多个结构体，一一对应 ABP 里的一组变量：

```
FAlsPoseState       ← PoseGrounded/Standing/Crouching/Gait... 曲线
FAlsGroundedState   ← VelocityBlend（四向）、MovementDirection、RotationYawOffsets、HipsDirection
FAlsFeetState       ← 每只脚的 Target/Lock/Final 位置与旋转、LockAmount、ThighAxis
FAlsTransitionsState← 排队的 Transition 序列与播放参数
```

动画蓝图用 PropertyAccess 节点直接读 `FeetState.Left.FinalLocation` 等字段，**不经过任何 UFUNCTION**。

### 6.4 Foot Lock 状态机（`:1246-1574`）

每帧每只脚执行固定三段：

1. `ProcessFootLockTeleport()` —— 若刚发生传送（0.2s 内），把锁点从 Component 空间取出重算；
2. `ProcessFootLockBaseChange()` —— 换平台时锁点转到新的 MovementBase 空间；
3. `RefreshFootLock()` —— 核心：

```cpp
if (NewLockAmount == 1 && 之前 < 1)  → 重新采样锁点（此刻脚才真正"落地"）
else if (!(NewLockAmount > 之前))    → 只允许减小（LockAmount 永不"淡入"）
```

即 **锁定的进入是瞬间的（等于 1 时采样），退出是渐变的**。再加两道约束 `ConstrainFootLock()`：

- `ThighDeltaAngle` 超过 `ThighAngleLimit` → 绕 Up 轴旋转锁点（防腿拧麻花）；
- `FootDeltaAngle` 超过 `FootAngleLimit` → 限制脚掌扭转。

另外还有一层自适应：角色开始移动时**主动把 LockAmount 拉低**（`:1401-1415`），不等动画曲线的值 —— 曲线只能"表达意图"，不能保证时序。

### 6.5 动态过渡 / 原地转身

- **Dynamic Transitions**（`:1683`）：脚被锁住但目标脚位已漂移超过阈值 → 播一个 additive 的"半步"过渡，带 2 帧冷却。这是"脚不能动但身体动了"的解药。
- **Turn In Place**（`:1904`）：视角 Yaw 超阈值且视角转速低于阈值 → 累计 `ActivationDelay`（延迟随角度映射）→ 选 90°/180° × 左右 × 站/蹲 → 排队播放。BlendOut 用 **Inertialization**，防止过渡改掉 `RotationYawSpeed` 曲线导致角色不再自行转身。
- **Rotate In Place**（`:1828`）：瞄准/第一人称下，播放速率随视角转速映射。

### 6.6 Linked Animation Instance

`UAlsLinkedAnimationInstance` 持有 `Parent` 弱指针，把父实例的初始化/刷新函数**重新暴露为 BlueprintThreadSafe UFUNCTION**（`:41-90`）。

用途：Overlay ABP（持枪/持弓/受伤）作为 Linked Layer 挂载，直接调用 `Parent->RefreshHead()` 等，**不必重复实现线程安全逻辑**，也不必把父实例的所有变量都暴露成 public。

---

## 7. 相机层：把相机做成"骨架网格体"

`UAlsCameraComponent : USkeletalMeshComponent`（`AlsCameraComponent.h:14`）—— 相机位置来自**一根有多根骨骼的相机骨架**，由相机 ABP 计算：

```
PivotTargetLocation  = (FirstPivotSocket + SecondPivotSocket) / 2      // 髋部中心
PivotLagLocation     = 按相机 Yaw 做轴向独立延迟
PivotLocation        = PivotLag + PivotOffset(曲线)
CameraTargetLocation = PivotLocation + CameraOffset(曲线)
CameraLocation       = 球扫收缩到最近合法点（TraceDistanceRatio）
```

三个工程要点：

1. `TickGroup = TG_PostPhysics` 且 `AddTickPrerequisiteActor(Owner)`（`:20, :48`）；
2. `CompleteParallelAnimationEvaluation()` 里补一次 `TickCamera`（`:100`）—— 并行动画求值完成后才能读曲线；
3. 站立于旋转平台时，相机旋转与 PivotLag 都存到平台空间（`:196-207`）。

第一人称不是"另一套相机"，而是 `FirstPersonOverride` 曲线在 FP socket 与 TP 位置之间做插值（`:220-232, :314-323`）—— **同一帧内完成 TP→FP 过渡**，避免了切换镜头带来的跳变。

---

## 8. 工具库：小而关键的 6 个文件

| 文件                           | 关键内容                                             | 为什么重要                                                             |
| ---------------------------- | ------------------------------------------------ | ----------------------------------------------------------------- |
| `AlsMath.h`                  | `DamperExactAlpha = 1 - exp(-ln2/halfLife * dt)` | **帧率无关**的插值；`FInterpTo` 在低帧率下近似瞬移                                 |
| `AlsRotation.h`              | `RemapAngleForCounterClockwiseRotation`          | 把 `(175°,180°]` 映射到 `(-185°,-180°]`，保证 180° 转身**始终逆时针**，否则 NaN/抖动 |
| `AlsVector.h`                | `DirectionToAngleXY` / `AngleToDirectionXY`      | 用角度代替向量做朝向计算，避免 `Atan2` 分支问题                                      |
| `AlsMontageUtility.cpp`      | `ExtractRootTransformFromMontage`                | 从蒙太奇任意时刻取 root transform（Mantle 必需）                               |
| `AlsPrivateMemberAccessor.h` | 私有成员访问模板                                         | 6.2 提到的 Hack                                                      |
| `AlsConstants.h`             | 全部骨骼/曲线/Slot 名集中                                 | 换骨架只改一处；避免 `FName` 拼写错误散布                                         |

---

## 9. 接入你的 ACT 项目：结论与风险

现状（已确认）：

```cpp
AACTCharacterBase   : public AAlsCharacter          // ACTCharacterBase.h:23
UACTGameAnimInstance: public UAlsAnimationInstance   // ACTGameAnimInstance.h:19
```

即**继承复用**，并非改造。以下 6 点是必须注意的。

### ① `LocomotionAction` 单值语义撑不住 ACT

ALS 假设"同一时刻只有一个全身动作"。ACT 需要：`攻击中可位移 / 可被击退 / 可取消接翻滚 / 可被浮空`。

建议：

- **保留** `LocomotionAction` 只表达"移动层被接管"的事实（翻滚/攀爬/布娃娃/起身）；
- 战斗动作状态交给 GAS（`GameplayAbility` 的 `ActivationOwnedTags` + `BlockAbilitiesWithTag`，你已启用 `GameplayAbilities`）；
- 若一定要在 ALS 内扩展，把 `FGameplayTag LocomotionAction` 换成 `TArray<FGameplayTag>` 或小栈，并同步修改 5 处 `IsValid()` 判断（第 2.4 节列表），**不要**在各处零散加 `if`。

### ② 旋转只有 4 个合法出口

弹反转身、受击转向、处决对齐 —— 一律走 `SetRotationInstant` / `SetRotationSmooth` / `SetTargetYawAngle`。直接 `SetActorRotation` 会在下一次 `RefreshGroundedRotation` 被覆盖，或在有 RootMotion 时产生跳变。

注意 `RefreshGroundedRotation()` 开头的 `HasAnyRootMotion()` 早退（`:1521-1525`）：**一旦播带 root motion 的蒙太奇，ALS 就放弃旋转控制**，改由蒙太奇决定。攻击动画如果要"边打边转"，必须用 Motion Warping 或自定义 RMS。

### ③ 改移动参数必须进 NetMoveData

见 4.1。攻击时若临时改 `MaxWalkSpeed` / `bInputBlocked` / `GroundFriction`：

- `bInputBlocked` 是本地状态，**不复制**，服务器靠 move data 里的 stance/gait 推导 —— 若你的动作改了速度但没进 move，联机必然拉扯；
- 目前 `FAlsSavedMove` 只带 3 个 tag，扩展时按同样套路加字段 + `NetSerializeOptionalValue`。

### ④ 项目里的 `UACTCharacterMovementComponent` 目前是"死代码"，且不能直接换上

```cpp
// Components/ACTCharacterMovementComponent.h:13
class UACTCharacterMovementComponent : public UCharacterMovementComponent
```

问题：

1. 全项目**没有任何引用**（仅自身文件）；
2. 它继承的是 `UCharacterMovementComponent`，不是 `UAlsCharacterMovementComponent`；
3. `AAlsCharacter` 构造函数（`:22-24`）已经把 CharacterMovement 子对象类设成 `UAlsCharacterMovementComponent`，随后 `AlsCharacterMovement = Cast<...>`（`:49`）并**无空指针检查**就调用 `AlsCharacterMovement->OnPhysicsRotation.AddUObject`（`:126`）、`SetMovementSettings`（`:130`）。

所以若哪天把它设为 CharacterMovement 类，`AAlsCharacter::PostInitializeComponents` 会**空指针崩溃**。

处理建议（二选一）：

- 若不需要自定义移动逻辑 → 删掉该文件，避免误用；
- 若需要 → 让它 `: public UAlsCharacterMovementComponent`，并在 `AACTCharacterBase` 构造函数里用 `ObjectInitializer.SetDefaultSubobjectClass<UACTCharacterMovementComponent>(CharacterMovementComponentName)` 正确替换。

### ⑤ 动画侧的线程安全红线

新增的战斗状态变量，若要在 ABP 中读取：

- 写 `NativeUpdateAnimation()` 或 `NativeThreadSafeUpdateAnimation()`；
- 绝不在 WT 里调用 `GetCharacter()->GetXxx()`（读 GT 数据，无同步）；
- 需要在 WT 播放蒙太奇 → 只能"排队"，在 `NativePostUpdateAnimation()` 里真正调（参考 `TransitionsState.Queued*` 模式）。

### ⑥ `bPendingUpdate` 与 Mesh Tick 策略别乱改

`RefreshMeshProperties()`（`:327-402`）会在 URO / 监听服务器等场景切换 `VisibilityBasedAnimTickOption` 与 `SetUsingAbsoluteRotation`。若你的项目自定义了 URO 或手动驱动 Mesh Tick，务必先读这段，否则会出现"转身动画与角色旋转失步"和"Foot Lock 滑步"。

---

## 10. 建议的代码阅读路线

**第一轮：建立骨架（约 1 天）**

1. `ALS.uplugin` + `README.md` → 知道有哪些模块
2. `AlsCharacter.h` 全文（690 行）→ 所有状态与 API 一览
3. `AlsCharacter.cpp:266-301`（Tick）→ 记住调用顺序
4. `AlsGameplayTags.h` → 状态维度

**第二轮：三个动作（约 2 天，收益最大）**  
5\. `AlsCharacter_Actions.cpp` 的 Roll → 学"预测 + 广播"  
6\. Mantle 全流程（含 `AlsRootMotionSource_Mantling.cpp`）→ 学自定义 RMS  
7\. Ragdoll 的 Start/Stop → 学物理态与胶囊态重建

**第三轮：动画与网络（约 2 天）**  
8\. `AlsAnimationInstance.cpp:98-235` → 线程模型  
9\. `RefreshFeet` / `RefreshFootLock` / `ConstrainFootLock` → 最难也最值钱  
10\. `AlsCharacterMovementComponent.cpp` 的 NetMoveData 与 `PhysWalking` 改动点  
11\. `RefreshViewNetworkSmoothing` → 视角平滑

**第四轮：按需**

- 相机 → `AlsCameraComponent.cpp:TickCamera`
- 编辑器自动化 → `ALSEditor/Modifiers/*`

---

## 11. 值得直接借鉴的 7 个工程技巧

1. **意图/结果双状态** + 只复制意图 —— 联机抖动的最小化方案。
2. **`FSavedMove` 装载影响预测的参数** —— 任何影响移动的临时状态都应如此。
3. **程序化 RMS 代替蒙太奇 root motion** —— 需要"可被打断 + 网络同步"的位移（ACT 的位移技、受击位移）强烈推荐。
4. **排队-回主线程**模式 —— WT 不能做的事（Montage、UObject 创建）统一在 PostUpdate 消费队列。
5. **`DamperExact` 替代 `FInterpTo`** —— 帧率无关插值，低端机上表现一致。
6. **曲线作为"表现参数总线"** —— C++ 只出数值，动画蓝图出感受，两边解耦。
7. **集中式常量表**（`AlsConstants.h`）—— 换骨架/改命名只动一处。
