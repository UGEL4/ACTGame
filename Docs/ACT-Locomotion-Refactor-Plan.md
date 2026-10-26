# ACT 自研 Locomotion 重构方案

> 目标：以 ALS-Refactored 为**参考**（而非继承），实现自己的 Character / MovementComponent / AnimInstance，
> 用 UE 5.8 的高级动画节点重建动画蓝图。
> 约束：**不使用 Motion Matching**（资源不足）、**暂不考虑网络同步**、**动画资源来自多个动画包，无统一参考姿势、无内置曲线**。

> ### ⚠️ 修订（2026-10-23）：实现层归属原则
>
> **节点能做的，一律在动画蓝图做；C++ 只保留节点做不到的。**
> 下文 §4 与「Phase 4 实现要点」里描述的 `AnimSpeed` / `StrideScale` / `OrientationAngle` /
> `BrakingDistance` / Lean / LandPredictionAlpha 的 C++ 实现**已全部下放 ABP**，
> 现在 C++ 只剩物理信号、权威状态、跨线程蒙太奇三类。
>
> 具体接线（Property Access 路径、节点引脚、Lean 公式、Distance Matching 参数）见
> **`Docs/ABP-v1-Wiring.md`**。那份文档才是当前的有效实现依据；下文保留作为设计推演过程。

---

## 0. 结论先行：从哪开始

**不要从动画蓝图开始。** 动画蓝图是这个系统最下游的一层，它的复杂度上限完全由上游的数据契约决定。
ALS 的 ABP 之所以可怕，一半原因不在动画节点，而在于它对上层提了 40+ 个隐式要求（每个 bool、每个 enum、每条曲线都是一份契约）。

正确的起手顺序是**倒着做**：

```
① 资产体检（统一骨架 / 摸清有无 root motion）
        ↓
② 数据契约（C++ 结构体，唯一接口，10~14 个字段）
        ↓
③ 移动组件（胶囊体位移的唯一权威）
        ↓
④ 角色类（输入 → 意图，相机）
        ↓
⑤ 动画蓝图 v1（只做 Idle + 走跑，验收"零滑步"）
        ↓
⑥ 起步 / 停止 / 转身（此时才有资格做，且不再靠 lean）
        ↓
⑦ 空中/落地/上半身槽位/攻击接入
```

理由：

- 步骤 ② 的字段列表是**一票否决项**。如果动画层将来要读"到落地还有多久""朝向与速度夹角多少度""刹车强度"，这些必须由 C++ 算好传下来。等 ABP 搭完再回头补，就是 ALS 那条路（曲线 + 布尔树）。
- 步骤 ③ 决定 ⑤ 能不能落地。速度分级、加速曲线、转向策略全在移动组件里；ABP 只负责"把速度翻译成姿势"。
- 步骤 ⑤ 用最小可跑版本验证管线通了（骨架对齐、Warping 节点生效、IK Rig 配置正确），再往上堆状态。
- 步骤 ⑥ 的"起步/停止/回转不自然"，**根因不是缺 lean，而是缺位移对齐**。详见 §2.3。

---

## 1. Step 0：资产体检（先做这个，1~2 天）

这一步不写代码，只出**一张表**，但它决定后面所有技术选型。

### 1.1 现状（已扫描）

| 目录 | 资源数 | 骨架 |
|---|---|---|
| `Content/MartialArtsFightGame/Anims` | 196 | `UE4_Mannequin_Skeleton` |
| `Content/InsaneAirCombo/Animation` | 141 | `UE4_Mannequin_Skeleton` / `reverse_grip_Skeleton` |
| `Content/Characters/Mannequins/Anims` | 90 | `UE4_Mannequin_Skeleton` |
| `Content/Blueprints/Characters/pl0001` | — | `Pl0001_Skeleton`（独立骨架） |
| `Content/UEFN_Mannequin` | — | `UEFN_Mannequin_Skeleton` |

**至少 4 套骨架。** 这直接判了 ALS 那套 `LayeredBlendPerBone`（靠固定骨骼名做姿态叠加）的死刑 —— 骨骼名对不上就叠不了。
同时 `no unified reference pose` 判了 **Additive Animation / Apply Additive** 的死刑 —— 加性动画必须有一致的参考姿势，否则叠加出的是垃圾。

### 1.2 必须确认的 4 件事

1. **每个 locomotion clip 有没有 root motion？**（`AnimSequence` 里看 `bEnableRootMotion`，或在视口播放看胶囊体是否移动）
   - 有 RM → 可走 **Distance Matching**（`UAnimDistanceMatchingLibrary`：`AdvanceTimeByDistanceMatching` / `DistanceMatchToTarget` / `SetPlayrateToMatchSpeed`），起步/停止可以做得非常自然。
   - 纯 in-place（动画包大概率如此）→ **放弃 Distance Matching**，改为 `Blend Space 速度轴 + Stride Warping + 手 K 的 start/stop clip`。这个判断必须在写 ABP 之前定下来，否则白搭。
2. **每个 skeleton 的骨骼命名是否一致？**（`pelvis` vs `Pelvis` vs `root` 层级差异）
3. **基础 locomotion 需要哪几条？** 建议先用一个角色（推荐 `UE4_Mannequin` 系，资源最多）跑通，其余角色靠 **IK Retargeter** 转。
4. **是否愿意给每个 clip 补一条曲线？** 你 `Build.cs` 里已经有 `AnimationModifiers` / `AnimationBlueprintLibrary`，可以写一个 `UAnimationModifier` 批量把 root motion 的位移烘焙成 `Distance` 曲线。**这是把"无曲线"变成"有曲线"的唯一低成本路径**，值不值得做，取决于第 1 条的答案。

### 1.3 产物

一张 Excel/表格：`clip 名 | 骨架 | 是否有 RM | 时长 | 起始/结束姿势 | 用途（idle/walk/run/start/stop/turn/jump/land）`。
这张表就是后面"哪条 clip 放进哪个 Blend Space"的唯一依据。

---

## 2. 为什么 ALS 的动画蓝图在你的项目里必然失控

逐条对照 ALS 的依赖点和你的现实，这决定了哪些能抄、哪些必须换。

### 2.1 曲线：ALS 把"表现参数"外置到了动画资产里

| ALS 依赖的曲线 | 用途 | 你的现实 | 替代 |
|---|---|---|---|
| `RotationYawOffsetCurve` | 地面朝向 = 相机 Yaw + 曲线（`AlsCharacter.cpp:1649`） | 动画包里没有 | 改为**运行时算**：C++ 输出 `YawOffsetDeg`，喂给 `Orientation Warping` 节点 |
| Stride Blend 曲线 | 步幅缩放 | 没有 | `Stride Warping` 按**实际速度**缩放，不需要预处理 |
| 各种 `Weight_*` 曲线 | 混合权重 | 没有 | 权重由 C++ 状态机给，不由曲线给 |
| Foot Lock 曲线 | 脚锁定 | 没有 | `Foot Placement` 节点（IK Rig 驱动，引擎内置） |

**结论：ALS 用曲线做的事情，UE 5.8 的 Warping 节点在运行时能做得更好且不需要美术预处理。**这是你最大的红利，应该吃干净。

### 2.2 叠加动画：没有统一参考姿势 → 结构上不可用

ALS 的分层方式是 `Layered Blend Per Bone` + `Apply Additive`（脊柱/头部叠加、上半身覆盖）。
前者只做骨骼空间混合，理论上不依赖参考姿势；**但后者（Additive）必须依赖**。

因此你的分层策略应该是：

```
                    ┌─ 上半身 Slot（蒙太奇整体覆盖）
Full Body 姿势 ─────┤
                    └─ 下肢 IK / Warping 后处理（程序化）
```

- **不要**"基础姿势 + 加性偏移"。
- **要**"完整 clip 之间切/混" + "上半身整段覆盖"。
- 这也意味着：攻击、受击这些动作**必须是完整身体 clip**（动画包里通常就是），刚好符合 ACT 的需求，比 ALS 那种"叠加式"更省事。

### 2.3 起步/停止/回转：lean 治标，位移对齐才治本

ALS 里 `Lean` 只是脊柱侧倾的角度（`AlsAnimationInstance.cpp` 的 `LeanAmount` 计算，来自横向速度/加速度）。
它不产生任何位移补偿，所以你看到的"不自然"是：

1. **起步**：clip 是"从静止蹬地"的固定长度，但角色速度是加速曲线爬升的 → 前 0.2 秒脚在原地蹭（滑步）。
2. **停止**：clip 假定"我在减速停下"，但胶囊体可能还在滑行，或者反过来 clip 播完了人还在滑。
3. **回转**：clip 假定"整体转 90°/180°"，但胶囊体是平滑插值转的，角速度曲线不匹配 → 脚跟着地面转，像在冰上。

**三个正交的解法（都要用）：**

| 问题 | 解法 | 说明 |
|---|---|---|
| 步幅与速度不匹配（滑步） | `Stride Warping` | 按 `LocomotionSpeed / PlaySpaceSpeed` 缩放腿长，运行时自适应 |
| 转身脚部打滑 | `Orientation Warping` | 把 root 旋转按比例分配到脊柱，脚留在地面 |
| 起停的残余位移 | `Offset Root Bone` | 累积 root 偏移量再平滑衰减，把"多出来的位移"吃掉，脚不滑 |
| 起停的时长对齐 | 专用 `start` / `stop` clip + 状态机 + 速度曲线 | **这一步不能省**，任何 Warping 都救不了"根本没有起步动作"的动画 |

> 关键认知：**lean 是"看起来有惯性"，Warping 是"脚真的踩在地上"。** 前者是美术，后者是物理。你缺的是后者。

### 2.4 决定：不用的 ALS 特性

既然不考虑网络同步，可以直接砍掉 ALS 里最麻烦的三块：

1. **`FAnimInstanceProxy` 的 PostUpdate 补丁**（ALS 为了在 Worker 线程播蒙太奇而打的自定义补丁，见 `AlsAnimationInstanceProxy.cpp`）—— 不需要。蒙太奇全部在 GT 播，动画实例只做纯数学。
2. **`FSavedMove` 那套预测参数**（`AlsCharacterMovementComponent.cpp:32-34`）—— 不需要。
3. **`Desired*` / 实际值的双状态**（`AlsCharacter.cpp:675-758`）—— 简化成单状态 + 平滑，网络同步才需要"意图 vs 结果"分离。

这一刀砍下去，代码量大概只剩 ALS 的 1/3，而且没有隐式约束。

---

## 3. 目标架构

### 3.1 三层职责边界（这是整个方案的地基）

| 层 | 类 | 只做 | 绝对不做 |
|---|---|---|---|
| 运动 | `UACTCharacterMovementComponent` | 速度、加速度、转向、重力、落地判定、地面信息 | 不碰动画，不认识 ABP |
| 意图 | `AACTCharacterBase` | 输入 → `DesiredGait` / 移动方向 / 动作请求；相机 | 不写动画逻辑，不直接改速度（走 `AddInputVector`） |
| 表现 | `UACTAnimInstance` | 读 C++ 快照 → 算混合权重 → 输出姿势 | 不写游戏逻辑，不读 GAS tag（只读已算好的层权重） |

### 3.2 数据契约（唯一接口）

`FACTLocomotionState` —— C++ 每帧填好，动画层只读。**只有这一个结构体跨界。**

字段设计原则：
- 动画层需要的一切都必须在这里，**不允许在 ABP 里调 `GetOwningActor` 再 cast 去取数据**（这就是 ALS 复杂度的来源之一）。
- 全部是**已算好的标量**，动画层不做物理推导（例如 `TimeToLand` 由 C++ 用 `VelocityZ / GravityZ` 算，不由动画层算）。
- 字段数控制在 14 个以内。多一个字段就是多一条 ABP 分支。

见 `Source/ACTGame/Locomotion/ACTLocomotionTypes.h`（已生成）。

### 3.3 类继承关系

```
ACharacter
 └─ AACTCharacterBase                    ← 自有，含相机 + 输入
      └─ AACTPlayerCharacter (可选)      ← 玩家特化

UCharacterMovementComponent
 └─ UACTCharacterMovementComponent       ← 复用现有空壳文件名，填充实现

UAnimInstance
 └─ UACTAnimInstance                     ← 自有，不再继承 UAlsAnimationInstance
```

`AACTCharacterBase` 构造函数里用 `SetDefaultSubobjectClass<UACTCharacterMovementComponent>(ACharacter::CharacterMovementComponentName)` 替换移动组件 —— 这是容易踩的坑，必须用 `SetDefaultSubobjectClass` 而不是 `CreateDefaultSubobject`。

---

## 4. 功能映射表：ALS → UE 5.8 节点

| ALS 的实现 | 新方案 | 节点/API | 备注 |
|---|---|---|---|
| 基础姿态混合树（几十个 bool） | 状态机 4 态：`Grounded` / `Jump` / `Fall` / `Land` | `AnimGraphNode_StateMachine` | 状态少是关键 |
| Gait 分层（Walk/Run/Sprint 三套） | 单个 **1D** Blend Space（仅速度轴）+ `Blend Space Graph` 复用 | `AnimGraphNode_BlendSpacePlayer` | 不加方向轴，理由见 §6 规则 2 |
| `RotationYawOffsetCurve` 决定朝向 | `Orientation Warping`，**Graph 模式**，`LocomotionDirection` ← `VelocityDirection` | `FAnimNode_OrientationWarping` | 有 RM 时用 Graph；Manual 作 fallback |
| 步幅随速度缩放（曲线） | `Stride Warping`，**Graph 模式**，`LocomotionSpeed` ← `Speed` | `FAnimNode_StrideWarping` | 节点自己用 `LocomotionSpeed / RootMotionDeltaSpeed` 算比例 |
| Foot Lock 自定义状态机 | `Foot Placement`，`PlantSpeedMode = Graph` | `FAnimNode_FootPlacement` | **有 RM 才可用**（Manual 模式要脚速曲线） |
| 起停残余位移 | **Distance Matching** + `Offset Root Bone` | `AdvanceTimeByDistanceMatching` / `DistanceMatchToTarget` | 需先烘焙 `Distance` 曲线 |
| Lean（脊柱侧倾） | **保留**，但只做惯性表现，不承担起停 | 纯 C++ 算角度 + `Control Rig` 或 `Modify Bone` | 别让它背锅 |
| 上半身覆盖 | `Slot` 节点（`UpperBody`）+ 蒙太奇 | `AnimGraphNode_Slot` | 单一 slot，不分 layer |
| 蒙太奇在 WT 播放（Proxy 补丁） | 全部 GT 播 | `UAnimInstance::Montage_Play` | 无网络同步 → 不需要 Proxy |
| Mantle 的 RootMotionSource | **保留思路** | 自定义 `FRootMotionSource` | 与动画解耦，仍然是最可控的位移方式 |
| 瞄准 70° 约束二次收敛 | 简化：`AimOffset` + 直接限制 | `Blend Space 1D` / `Aim Offset` | 不做迭代收敛 |
| 视角平台空间平滑 | 简化：相机直接跟随 | SpringArm | 不做平台空间处理 |

### 4.1 关于 `AnimationWarping` 的前置条件（踩坑预警）

`Orientation Warping` / `Stride Warping` / `Foot Placement` **都依赖 IK Rig 资产**（`IKRigDefinition`）。
即：每个角色骨架都要建一个 IK Rig，指定：
- `Pelvis` 骨
- 左右脚骨 + 脚趾骨
- 脊柱链（Orientation Warping 用）

**这是 Phase 4 之前必须完成的准备工作，且容易被低估。** 建议先给主角骨架建一个，验证通了再批量。

### 4.2 动画有 root motion → 三个结论要改

已确认 `UAnimSequence::GetAnimationPose`（`AnimSequence.cpp:1702-1713`）在 `HasRootMotion()` 为真时会把
每帧的 root motion delta 采样进 attribute 容器，且**这段不在 `WITH_EDITOR` 块内**，
provider 由 AnimationWarping 运行时模块注册 —— 所以 **Graph 模式打包后同样生效**，不是编辑器限定功能。

据此修正三条：

**① `Foot Placement` 从"不可用"变成"可用"。**
之前结论作废的原因是没有输入：`PlantSpeedMode = Manual` 要脚速曲线，`Graph` 要 RM delta。
现在直接把 `PlantSpeedMode` 设成 `Graph` 即可，**不需要烘焙 `FootSpeed_L/R` 曲线**。

**② Warping 节点优先用 Graph 模式。**
它用的是动画里真实的 `RootMotionDeltaSpeed` 和 root motion 方向，比 C++ 里估的 `AnimSpeed` 准。
代价是要求混合链上每个 clip 都有 RM —— 混进一个无 RM 的 clip，那一帧 delta 为 0，节点退回"不扭曲"。

**③ 起步/停止改用 Distance Matching，这是有 RM 后最大的收益。**
`AdvanceTimeByDistanceMatching` 按"已走距离"推进动画，`DistanceMatchToTarget` 按"到停止点的剩余距离"
设置动画时间 —— 两者都不再依赖时间轴，滑步从原理上被消除。
前置条件是 `Distance` 曲线，而 `AnimationLocomotionLibrary` 插件自带
**`UDistanceCurveModifier`**（`Source/Editor/Public/DistanceCurveModifier.h`），
右键动画 → `Add Animation Modifier` 选它即可批量烘焙，**不需要自己写 `UAnimationModifier`**。

它的两个开关值得注意：
- `CurveName` 默认 `"Distance"`
- `bStopAtEnd`：勾上则把 clip 末尾视为停止点（停止类动画要勾），否则按 `StopSpeedThreshold`（默认 5cm/s）自动判定
- 曲线语义：**正值 = 从起点已走的距离；负值 = 到停止点剩余的距离**，所以同一个 clip 可以同时服务起步和停止

---

## 5. 分阶段实施

每阶段结束都有**可验收的结果**，且不依赖下一阶段。

### Phase 0：资产归一（1~2 天，无代码）
- [ ] 完成 §1.3 的表格
- [ ] 确定主用骨架（建议 `UE4_Mannequin`，资源最多）
- [ ] 建 1 个 IK Rig（主角骨架）
- [ ] 决定是否需要批量烘焙 `Distance` 曲线（Animation Modifier）

**验收**：能明确回答"起步/停止是否走 Distance Matching"。

### Phase 1：数据契约（0.5 天）
- [ ] `ACTLocomotionTypes.h`：`EACTStance` / `EACTGait` / `EACTLocomotionMode` / `EACTRotationMode` / `FACTLocomotionState` / `FACTGaitSettings`
- [ ] 跑通编译（哪怕没有任何类使用它）

**验收**：字段列表定稿，后面不再加字段（加字段 = 重做 ABP 分支）。

### Phase 2：移动组件（2~3 天）
- [ ] 继承 `UCharacterMovementComponent`，删掉现有空壳里的错误继承
- [ ] 速度分级：`Walk / Run / Sprint` 三档，由 `MaxWalkSpeed` 按 gait 切换
- [ ] 加速曲线：`UCurveFloat` 驱动 `MaxAcceleration`（起步不再线性）
- [ ] 转向策略：`VelocityDirection`（朝速度）/ `LookingDirection`（朝相机），实现一个 `SmoothYawTo()`
- [ ] 地面信息：`DistanceToGround`、`GroundNormal`、`bOnWalkableFloor`
- [ ] 跳跃分段 + `TimeToLand` 预测
- [ ] 刹车强度 `BrakeAmount`（`1 - CurrentSpeed/PrevSpeed` 平滑）
- [ ] **关掉 `bOrientRotationToMovement` 和 `RotationRate`** —— 旋转只由 `SmoothYawTo` 一处负责（学 ALS 的"单一权威"）

**验收**：白盒调试能画出"速度 / 加速度 / 偏航角速度"三条曲线，且起步是 S 形加速。

### Phase 3：角色类（1~2 天）
- [ ] `AACTCharacterBase : public ACharacter`（脱离 `AAlsCharacter`）
- [ ] 构造函数替换移动组件类
- [ ] 输入：Move / Look / Sprint / Walk / Jump / Crouch / Attack
- [ ] 相机：SpringArm + Camera（先不用 `UAlsCameraComponent`，它与 ALS 设置耦合）
- [ ] 对外暴露 `SetDesiredGait()` / `SetDesiredStance()`，内部平滑到实际值

**验收**：能跑能跳能转身，角色胶囊体行为正确（此时还没有像样的动画）。

### Phase 4：动画蓝图 v1（2 天）★ 最关键
只做 **Idle + Walk/Run**，不做起停转。
- [x] `UACTAnimInstance`：`NativeUpdateAnimation`（GT）拷贝快照 → `NativeThreadSafeUpdateAnimation`（WT）纯计算
- [ ] ABP v1 拓扑（见 §6）
- [ ] 两个 Warping 节点用 **Graph 模式**（有 RM 了）；若混入无 RM 的 clip 就退回 Manual
- [ ] 接 `Foot Placement`（`PlantSpeedMode = Graph`）—— 有 RM 后可用，不需要烘焙脚速曲线

**验收**：**在 0/150/300/600 速度下走直线，脚不打滑。** 这条不通过，不要进 Phase 5。

### Phase 5：起步 / 停止 / 转身（3 天）★ 有 root motion → 用 Distance Matching
- [ ] 用 `UDistanceCurveModifier` 批量烘焙 `Distance` 曲线（右键动画 → Add Animation Modifier）
- [ ] 状态机加 `Start` / `Stop` / `Turn` 子状态（用 `Sequence Evaluator` 而非 `Sequence Player`）
- [ ] 起步：`AdvanceTimeByDistanceMatching`，`DistanceTraveled` = 本帧水平位移距离
- [ ] 停止：`DistanceMatchToTarget`，`DistanceToTarget` ← `BrakingDistance`（C++ 已算好）
- [ ] `Offset Root Bone` 吃掉残余位移（`Mode = Graph`，同样吃 RM attribute）
- [ ] 转身：`Orientation Warping` + 转身阈值分档（<45° 用 warp，>45° 切 turn clip）

**验收**：加速起步、急停、90°/180° 急转，脚部无滑动，减速时身体前倾自然。

**为什么不按时间轴播**：`Sequence Player` 是"时间驱动"，`SetPlayrateToMatchSpeed` 只能对恒定速度的循环动画做近似。
`Sequence Evaluator` + Distance Matching 是"距离驱动"——动画进度直接由位移决定，原理上不可能滑步。
这也是 Epic 在没有 Motion Matching 的情况下推荐的方案，而你已经有 root motion，正好能做。

#### 停止距离不要自己推导（`AnimCharacterMovementLibrary`）

`AnimationLocomotionLibrary` 插件里有两个 `BlueprintThreadSafe` 函数，是 Epic 专门为这件事写的：

| 函数 | 用途 | 输入来源 |
|---|---|---|
| `PredictGroundMovementStopLocation` | 从当前速度刹停会走多远、往哪个方向 | `Velocity` + CMC 的 `GroundFriction` / `BrakingFriction` / `BrakingFrictionFactor` / `bUseSeparateBrakingFriction` / `GetMaxBrakingDeceleration()` |
| `PredictGroundMovementPivotLocation` | 方向反转时"真正掉头"发生在哪一点 | `Acceleration` + `Velocity` + `GroundFriction` |

`PredictGroundMovementStopLocation` 与 `UCharacterMovementComponent::ApplyVelocityBraking` 是**同一套公式**
（`AnimCharacterMovementLibrary.cpp:7-30`）：

```
摩擦   = (bUseSeparateBrakingFriction ? BrakingFriction : GroundFriction) × BrakingFrictionFactor
T      = Speed / (摩擦 × Speed + BrakingDeceleration)
停止点 = V×T + 0.5×(-摩擦×V - BrakingDecel×Dir)×T²
```

本组件把 `BrakingFrictionFactor` 置 0，摩擦项归零，上式化简后**正好是 `v² / 2a`** ——
所以两种写法当前数值完全一致，`UACTAnimInstance` 里改用引擎函数是为了以后调摩擦时不产生第二套算法。

`PredictGroundMovementPivotLocation` 留给 Phase 5 的 `Turn` 状态：它告诉你在急转输入下，
胶囊体速度方向翻转完成的那个点离角色有多远，正好是转身动画该"蹬地"的位置。
注意它要的是 `Acceleration`，动画层里应读移动组件缓存的 `GetCurrentAcceleration()`，
**不能**在 `NativeUpdateAnimation` 里现算（输入已被消费）。

### Phase 6：空中 / 落地 / 上半身 / 攻击接入（3~5 天）
- [ ] `Jump` / `Fall` / `Land` 三态 + 落地预测（`TimeToLand` 驱动落地预备姿势）
- [ ] `UpperBody` Slot 节点 + 蒙太奇按 slot 播
- [ ] 攻击蒙太奇接入 `ActionLogicComponent`（在 GT 播，不走 Proxy）
- [ ] 战斗状态（受击/硬直）由 GAS `ActivationOwnedTags` 驱动，**只向动画层暴露 3~4 个层权重**（不用几十个 bool）

**验收**：攻击中可位移、可被打断、可接翻滚，动画层不打架。

---

## 6. 动画蓝图 v1 具体拓扑

UE 的 AnimGraph 是**自下而上求值**：最靠近 Root 的节点最后执行。下面按"从上往下读 = 应用顺序"书写。

```
[Event Graph]
  NativeThreadSafeUpdateAnimation 已在 C++ 完成，这里不写逻辑
  Blend Space 的 X 轴直接连 AnimSpeed

[AnimGraph]  ★ 推荐：动画有 root motion → 两个 Warping 都用 Graph 模式
Root
 └─ Slot "UpperBody"              ← Phase 6 再接，v1 留空
     └─ Layered Blend Per Bone（仅 1 根：spine_01，**不加 additive**）
         └─ Base Pose: 全身 Locomotion
             └─ Stride Warping         Mode = Graph
             │                          LocomotionSpeed    ← Speed
             │                          StrideDirection    ← VelocityDirection（或 ForwardVector）
             │                          MinRootMotionSpeedThreshold = 10
             └─ Orientation Warping    Mode = Graph
             │                          LocomotionDirection ← VelocityDirection
             │                          TargetTime = 0.8（前瞻，避免起步瞬间误扭曲）
             │                          MinRootMotionSpeedThreshold = 10
             │                          RotationInterpSpeed = 10
                 └─ State Machine
                     ├─ Grounded
                     │    └─ Blend Space **1D** (仅 Speed 轴，接 AnimSpeed)
                     │        （Idle / Walk / Run 全在一个 BS 里）
                     ├─ Jump   → Sequence Player
                     ├─ Fall   → Sequence Player
                     └─ Land   → Sequence Player
```

**如果主角色的部分 clip 没有 root motion**（混用多套动画包时很常见），改成 Manual 模式，
把 `StrideScale` / `OrientationAngle` 两个引脚接回 C++ 输出即可，拓扑不用动：

| | Graph（推荐，有 RM） | Manual（fallback，无 RM） |
|---|---|---|
| Orientation Warping | `LocomotionDirection ← VelocityDirection` | `OrientationAngle ← OrientationAngle` |
| Stride Warping | `LocomotionSpeed ← Speed` | `StrideScale ← StrideScale` |
| 优点 | 用动画真实位移，最准 | 只依赖标量，任何 clip 都能用 |
| 缺点 | 要求链上每个 clip 都有 RM | 比例是估算的 |

**五条硬性规则（违反任何一条都会退化成 ALS 那种复杂度）：**

1. **一个 Blend Space 覆盖所有地面速度，且只用 1D（Speed 轴）。** 不要每个 gait 一套。

2. **不要给 Blend Space 加 Direction 轴。** 这一条是这次实现才明确的，很容易踩：
   `Direction`（= 速度相对角色前向的夹角）在**转身过程中会冲高再回落** ——
   摇杆从"前"推向"右"，速度方向瞬间变成 90°，而角色朝向要 0.33 秒才转过去，
   于是 Direction 先在 0.1 秒内冲到 90°、再慢慢回到 0°。2D Blend Space 会跟着这个值
   播一遍"右移动画"，和 Orientation Warping 的转向**叠加成双重转向**，表现为角色一边转身一边横着走。
   正确分工是：**Blend Space 只管"多快"（永远播前进动画），Orientation Warping 管"朝哪"**。
   一套前进动画就能覆盖全方向移动，这正是 Orientation Warping 存在的理由。

3. **节点顺序固定**（自下而上）：`State Machine → Orientation Warping → Stride Warping → Slot → Root`。
   - 两个 Warping 都作用在 locomotion 姿态本身，必须在 Slot 之前，否则蒙太奇播放时会被扭曲。
   - 不要把 Foot Placement 加进 v1：它的 `PlantSpeedMode` 默认 `Manual`，
     需要每只脚的 `FKSpeed` 曲线；切成 `Graph` 又需要 root motion delta 属性。
     当前动画包两者都没有，节点会静默不工作 —— 看起来像"配错了"，实际是缺输入。
     处理办法见 §7。

4. **Slot 只做"整体覆盖"**，不用 `Layered Blend Per Bone` 做姿态叠加。

5. **Event Graph 里不写 cast、不写分支**。所有判断在 C++。

---

## 7. 已知坑 / 风险

| 风险 | 影响 | 应对 |
|---|---|---|
| Warping 节点需要 IK Rig，配置错会静默失效 | 症状是"看起来没效果"，很难查 | Phase 4 先用极简 IK Rig 验证单个节点生效，再串联 |
| `Foot Placement` 与 `Control Rig` 顺序冲突 | 脚悬空或穿地 | Foot Placement 之后不要再改 root/pelvis |
| Graph 模式混进一个无 RM 的 clip | 那一帧 delta=0，节点静默退回"不扭曲"，表现为偶发滑步 | 同一个 Blend Space 里只放同源动画；混用多套包时整体退回 Manual 模式 |
| 部分 clip 无 RM | `Offset Root Bone` / Distance Matching 无输入 | 按 clip 分组，有 RM 的走 Graph，没有的走 Manual，不要在一个混合链里混 |
| 4 套骨架，Retarget 后脚部尺寸不一致 | 脚滑 | 用 IK Retargeter + `Foot Placement` 吸收，不要指望骨骼比例完全一致 |
| 一次性删 ALS 导致全部 BP 断裂 | 项目无法启动 | **分两步**：先新建类并迁移 BP，验证通过后再从 `Build.cs` 移除 `ALS`/`ALSCamera` |
| `UACTCharacterMovementComponent` 现有版本是死代码 | 换上去会崩（Cast 空指针解引用） | 直接重写该文件，不要保留原内容 |

---

## 8. 文件清单与进度

```
Source/ACTGame/Locomotion/
  ACTLocomotionTypes.h                  ← Phase 1 ✅ 数据契约
  ACTCharacterMovementComponent.h/.cpp  ← Phase 2 ✅ 移动组件

Source/ACTGame/Character/
  ACTCharacterBase.h/.cpp               ← Phase 3 ✅ 已改为继承 ACharacter，脱离 ALS

Source/ACTGame/Animation/
  ACTAnimInstance.h/.cpp                ← Phase 4 ✅ 继承 UAnimInstance，脱离 ALS
  ACTGameAnimInstance.h/.cpp            ← 旧的 ALS 派生类，等 BP 迁移完再删
```

已删除 `Source/ACTGame/Character/Components/ACTCharacterMovementComponent.h/.cpp`
（空壳死代码，继承 `UCharacterMovementComponent` 但零引用；同名 UCLASS 会和新的冲突）。

### Phase 2 实现要点（已完成）

- 旋转唯一入口 `PhysicsRotation()`，已关闭 `bOrientRotationToMovement` / `bUseControllerDesiredRotation`，
  `BeginPlay` 里同时关闭 Character 的 `bUseControllerRotationYaw`（默认 true，会和平滑转向打架）。
- **`BrakingFrictionFactor = 0`**：`ApplyVelocityBraking` 的公式是 `v += (-Friction·v + RevAccel)·dt`，
  默认 `GroundFriction(8) × BrakingFrictionFactor(2)` 的指数项会在 0.06 秒内吃光速度，
  导致 `BrakingDecelerationWalking` 完全失效、`TimeToStop` 填什么都没用。置 0 后只剩恒定减速度。
- `MaxAcceleration = MaxSpeed / TimeToMaxSpeed`，`BrakingDecelerationWalking = MaxSpeed / TimeToStop`。
- `MaxWalkSpeed` 不直接跳变，按 `GaitTransitionTime` 插值，避免跑步切走路被 `IsExceedingMaxSpeed` 硬钳。
- `MoveInput` 用 `GetLastMovementInputVector()` 而非 `GetCurrentAcceleration()`（后者在 `PerformMovement` 后可能已被清空）。
- `AccelerationMagnitude` / `BrakeAmount` 用**速度实际变化率**算，不用期望加速度（有摩擦/斜坡/撞墙时才不抖）。
- `TimeToLand` 用重力公式推导（`2v/g` 或 `|v|/g`），不打射线；`DistanceToGround` 用 10Hz 限频射线。

### Phase 3 实现要点（已完成）

- `AACTCharacterBase` 改为 `public ACharacter`，构造函数用
  `Super(ObjectInitializer.SetDefaultSubobjectClass<UACTCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))`
  替换移动组件。**不能用 `CreateDefaultSubobject`**，否则会有两个移动组件，且引擎用的仍是基类实例。
- 相机改为 SpringArm + Camera（过肩偏移 `SocketOffset(0,40,60)`、`FOV 90`、位置 lag 12、无旋转 lag）。
  原 `UAlsCameraComponent` 已移除 —— 实测它只 `Cast<ACharacter>`（`AlsCameraComponent.cpp:37`），
  并不强依赖 `AAlsCharacter`，但需要 `UAlsCameraSettings` 资产否则 `ALS_ENSURE` 报错，
  想换回来自建组件即可。
- 视角改为直接写 `ControlRotation` + `FMath::ClampAngle(MinLookPitch, MaxLookPitch)`，
  不用 `AddControllerYawInput/PitchInput`（会乘 `InputYawScale_DEPRECATED`，还得手工改成 1.0）。
- 移动方向只用 Yaw 构造 `FRotationMatrix`，避免相机俯仰时前进方向带 -Z 分量导致速度被归一化。
- **蹲下必须开 `GetNavAgentPropertiesRef().bCanCrouch = true`**：`FNavAgentProperties::bCanCrouch`
  默认 false，不开的话 `ACharacter::Crouch()` 会静默失败（`CanCrouch()` 返回 false）。
- 绑定表沿用 ALS 写法：`Triggered` + `Canceled` 成对绑定（Canceled 负责松手归零），
  `Walk` / `Crouch` / `Roll` 绑 `Triggered` 即可。
- `PostInitializeComponents` 里缓存 `ACTMovement`，失败时打 Error 日志（防止蓝图覆盖组件类）。

#### 还需要的资产（Phase 3 自测前）

- 新建 `IA_Sprint` / `IA_Walk` / `IA_Crouch` / `IA_Roll`（现有只有 `IA_Move` / `IA_Look` / `IA_MouseLook` / `IA_Jump` / `IA_ButtonX` / `IA_ButtonY`）。
- 在 `IMC_Default`（或新建 `IMC_ACT`）里加映射。
- 新建测试蓝图 `BP_ACTCharacter`（父类 `AACTCharacterBase`），挂骨架网格 + 赋 IMC 与各 InputAction。
- 注意：`BP_Pl0001_New` / `BP_Pl0001_New1` 的父类是 `AACTGamePlayerBase`（另一个简单 `ACharacter`），
  没有被这次改动影响，也不要急着 reparent，等 Phase 4 动画实例就绪再迁。

### Phase 4 实现要点（已完成）

**1. 用 AnimSpeed + StrideScale 组合替代 Distance Matching**

这是整份方案里最关键的一处设计，直接对应"起步/停止不自然"：

- `AnimSpeed` = `LocomotionState.Speed` 的**滞后版本**（`FInterpTo`，默认 8/s），喂给 Blend Space。
- `StrideScale` = `Speed / AnimSpeed`，喂给 Stride Warping。
- 起步：`AnimSpeed < Speed` → `StrideScale > 1` → 步幅放大，脚真的往前踩。
- 急停：`AnimSpeed > Speed` → `StrideScale < 1` → 步幅缩到接近 0，脚被按住不动。
- 稳态：两者相等 → `StrideScale = 1`，节点不干预。

为什么不直接用 Speed：速度是物理量，可以瞬时变化（撞墙、急停、被击退）。直接喂给 Blend Space，
姿势就会跟着瞬时跳变，观感像"被拽着走"。这个滞后量就是给动画留的反应时间，而位移差额由 IK 补掉。
效果等价于 Distance Matching，但**完全不需要 root motion**。

**2. 两个 Warping 节点都必须用 Manual 模式**

查引擎源码确认（`AnimNode_StrideWarping.cpp:203-210`）：

```cpp
// Graph 模式才会用 LocomotionSpeed 自动算 StrideScale
ActualStrideScale = LocomotionSpeed / CachedRootMotionDeltaSpeed;
```

Graph 模式要求 root motion delta 属性（`IAnimRootMotionProvider::ExtractRootMotion`），in-place 动画没有，
节点会直接 early return。所以必须：

| 节点 | 模式 | 引脚 | 数据来源 |
|---|---|---|---|
| Orientation Warping | `Manual` | `OrientationAngle` | `VelocityYawOffset * OrientationAngleScale` |
| Stride Warping | `Manual` | `StrideScale` | `Speed / AnimSpeed` |

Orientation Warping 的 Manual 分支是 `TargetOrientationAngleRad = DegreesToRadians(NormalizeAxis(OrientationAngle))`
（`AnimNode_OrientationWarping.cpp:377`），即**直接就是绕 Z 的旋转角度**，没有额外换算。

保底手段：`OrientationAngleScale` 默认 1，发现腿转向反了就填 `-1`，不用改 C++ 重编。

**3. 蒙太奇用队列，不用 Proxy 补丁**

ALS 为了在 WT 上播蒙太奇，给 `FAnimInstanceProxy::PostUpdate` 打了补丁（`AlsAnimationInstanceProxy`）。
不做网络同步后，这里只需要一个 `TQueue<FActPendingMontage, EQueueMode::Mpsc>`：

- `PlayMontageSafe()`：`IsInGameThread()` 为真就立即 `Montage_Play`，否则入队并返回 0。
- `NativeUpdateAnimation`（GT）每帧 flush 队列。
- 停止请求单独一个队列，且**先处理停止再处理播放** —— 否则同一帧里"取消上一招 + 接下一招"会把新招也停掉。

代价：从 WT 发起的播放会延后一帧。实际影响可忽略，因为动作系统（Phase 6）全在 GT 跑。

**4. WT-only 成员不要跨线程读**

`Speed` / `AnimSpeed` / `StrideScale` / `OrientationAngle` / `Lean` 都只在
`NativeThreadSafeUpdateAnimation` 里写。GT 侧若要数据，读 `LocomotionState` 而不是这些派生量 ——
GT 读 WT 正在写的变量是数据竞争，编译期不报错，运行时表现为偶发的数值跳动。

`LocomotionState` 是 UPROPERTY，GT 写完后 GT→WT 之间有一次同步，WT 读它是安全的。

**5. 退化路径**

`NativeInitializeAnimation` 找不到 `UACTCharacterMovementComponent` 时（例如新 AnimBP 挂在旧角色上），
打 Warning 并用 `Character->GetVelocity()` 直接填 `Speed` / `VerticalSpeed` / `VelocityYawOffset` / `Mode`。
迁移期挂着新 ABP 也能看姿势，不至于全零。

### Foot Placement 的输入问题（有 RM → 已解决）

`FAnimNode_FootPlacement::PlantSpeedMode` 默认 `Manual`，速度来自
`FFootPlacemenLegDefinition::SpeedCurveName` 指定的曲线；切 `Graph` 则从 root motion delta 取。

既然动画有 root motion，**直接设 `PlantSpeedMode = Graph`，不需要烘焙 `FootSpeed_L/R` 曲线**。
`Manual` 模式只在"该 clip 确实没有 RM"时才需要，那时再考虑用 `UAnimationModifier` 烘焙脚速曲线。

**另外 `Foot Placement` 要勾 `bEnabled`，且 `Trace Settings` 的 `StartOffset`（默认 -75）以脚骨为基准，
不同骨架的身高比例不一样，这个值要按实际调** —— 偏了的表现是脚悬空或穿地，而且不报错。
