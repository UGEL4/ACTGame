// ACT 移动组件 —— 胶囊体位移与朝向的唯一权威
//
// ================================================================ 它做什么
//   1. 位移：输入 → 加速度/减速度 → 速度 → 移动。所有速度参数都是"时间"换算出来的。
//   2. 朝向：两种驱动方式可切换（见 EACTRotationAuthority）——
//        MovementComponent：RotationRate.Yaw 限速，目标按「输入 → 速度 → 保持」选。
//                           **松手后仍用速度方向续转**，所以点按一下也能把身转完。
//        AnimationCurve：ABP 推偏航偏移，组件直接落位，节奏由动画曲线决定。
//   3. 快照：把动画层需要、而动画层算不出来的量组装成 FActLocomotionState。
//
// ================================================================ 它不做什么（重要）
//   1. **不判定转身**：没有 pivot 请求、没有大角度提速、没有速度折扣、没有冷却。
//      "什么时候播转身/折返动画、转多少度"是动画层的决定，见
//      Docs/Turn-Pivot-CurveDriven-Design.md（§4 曲线契约 / §8.5 瞬转折返）。
//   2. **不碰动画资产**：不认识 Montage，不认识曲线。唯一例外是曲线模式下的偏航通道，
//      那是一个 atomic 槽，不涉及资产。
//   3. **不做网络预测**：没有 FSavedMove、没有 Desired*/实际值双状态。
//      曲线模式是纯客户端表现，专用服务器上没有 AnimInstance 会自动退回 MovementComponent。
//
// ================================================================ 三条容易踩的坑（已处理，别改回去）
//   1. 旋转权威唯一：两个引擎自带的 bool（bOrientRotationToMovement / bUseControllerDesiredRotation）
//      恒为 false，旋转由本类的 PhysicsRotation 独占。别去开它们 —— 一开就会和这里的实现抢，
//      而且引擎那条路在"松手"时会放弃旋转（见构造函数注释）。
//      Character 的 bUseControllerRotation* 必须全关，否则引擎会越过 RotationRate 直接吸附。
//   2. BrakingFrictionFactor 必须为 0：否则 ApplyVelocityBraking 里的指数摩擦项会盖掉
//      恒定减速度，TimeToStop 形同虚设（见 .cpp 构造函数注释）。这个值同时也是
//      距离匹配（PredictGroundMovementStopLocation）能算准的前提。
//   3. RotationRate.Yaw 是运行时调速的唯一入口，别再加一个"插值速度"字段做同一件事。
//
// 结构对照 ALS-Refactored 的 CMC，但剥离了网络同步、转身动作与蒙太奇。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Locomotion/ACTLocomotionTypes.h"
#include "ACTCharacterMovementComponent.generated.h"

class UACTAnimInstance;

DECLARE_LOG_CATEGORY_EXTERN(LogACTMovement, Log, All);

UCLASS()
class ACTGAME_API UACTCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UACTCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 朝向的唯一写入口。MovementComponent 模式按 RotationRate 限速插值，曲线模式直接落位。 */
	virtual void PhysicsRotation(float DeltaTime) override;

	/** 落地钩子：置 Landing 计时，动画层据此播落地缓冲。 */
	virtual void ProcessLanded(const FHitResult& Hit, float RemainingTime, int32 Iterations) override;

	// ================================================================ 步态 / 站姿 / 朝向模式

	/** 切换步态。只改目标最大速度，实际速度由加速度平滑过渡（不会瞬移）。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetGait(EACTGait NewGait);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	EACTGait GetGait() const { return Gait; }

	/** 切换站姿。走引擎的 crouch 机制（会改胶囊体高度），动画层从 bIsCrouched 推导。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetStance(EACTStance NewStance);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	EACTStance GetStance() const;

	/** 朝移动方向 / 朝相机。只在 MovementComponent 模式下影响旋转；曲线模式忽略它。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetRotationMode(EACTRotationMode NewMode);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	EACTRotationMode GetRotationMode() const { return RotationMode; }

	// ================================================================ 旋转权威

	/**
	 * 切换旋转由谁驱动。切到 AnimationCurve 等价于调 BeginAnimationDrivenYaw。
	 * @see EACTRotationAuthority
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetRotationAuthority(EACTRotationAuthority NewAuthority);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	EACTRotationAuthority GetRotationAuthority() const { return RotationAuthority; }

	/**
	 * 进入曲线模式（幂等）。以**当前朝向**为基准角，之后 ABP 推的偏移都相对它。
	 * 曲线首帧的偏移应当为 0，否则第一帧会跳。
	 *
	 * 配套的 ABP 侧：`PushYawCurveOffset(度)` 每帧推，`ClearYawCurveOffset()` 结束时清。
	 * 两者不成对也不会卡死 —— AnimationYawTimeout 会兜底。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void BeginAnimationDrivenYaw();

	/** 退出曲线模式（幂等）。朝向不跳：下一帧由 MovementComponent 从当前角继续。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void EndAnimationDrivenYaw();

	/**
	 * 硬设朝向，立刻生效，不受 RotationRate 或曲线限制。
	 * 折返（瞬转 + 刹车）、翻滚起手、攻击起手用。
	 * 曲线模式下调用会把基准角一起搬到新朝向，否则下一帧会被曲线拉回去。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SnapYawTo(float NewYaw);

	/**
	 * MovementComponent 模式下是否允许引擎自动刷新朝向（默认 true）。
	 * 关掉后引擎完全不转 —— 攻击锁定朝向、处决动画用。
	 * 注意这跟"冻结在某个角"不同：它冻结的是"转向"这个行为，不是某个数值。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetAutoYawUpdate(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	bool IsAutoYawUpdateEnabled() const { return bAutoUpdateYaw; }

	// ================================================================ 动画层数据

	/** 动画层唯一数据源。
	 *  注意：不加 UFUNCTION —— UFUNCTION 不能返回引用，这里必须是零拷贝的 C++ 访问。 */
	const FActLocomotionState& GetLocomotionState() const { return LocomotionState; }

	const FActGaitSettings& GetGaitSettings(EACTGait InGait) const { return GaitSettings.Get(InGait); }

protected:
	// ================================================================ 内部更新

	/**
	 * 按 RotationMode 把 RotationRate.Yaw 刷成对应档位的角速度。
	 * 注意：两个引擎自带的 bool（bOrientRotationToMovement / bUseControllerDesiredRotation）
	 * 在构造函数里就被关成 false 了，本项目完全接管 PhysicsRotation，不再走引擎那条路。
	 */
	void RefreshRotationRate();

	/**
	 * 每帧推导目标偏航角。必须在 Super::TickComponent 之前调用。
	 *
	 * **这个函数是"点按手感"的全部来源**：目标角是持久状态，松手不会清空。
	 * 如果改成每帧现算（没有状态），松手后速度 14ms 就归零，目标立刻丢失，
	 * 点按一下只能转 18°（≈ 9°/帧 × 2 帧），到不了 90°。
	 */
	void UpdateDesiredYaw();

	/** 曲线模式：从动画实例拉最新偏移并落位。超时/被清除时自动退回 MovementComponent。 */
	void ApplyAnimationDrivenYaw(float DeltaTime);

	/** 解析并缓存 AnimInstance。没有动画实例（专用服务器）时返回 nullptr。 */
	UACTAnimInstance* ResolveAnimInstance();

	/** 把 GaitSettings 里的时间换算成 MaxAcceleration / BrakingDecelerationWalking。 */
	void UpdateGaitParameters(float DeltaTime);

	/** 组装给动画层的只读快照。在 Super::TickComponent 之后调用，读到的是本帧移动结果。 */
	void UpdateLocomotionState(float DeltaTime);

	/** 空中时探测脚底到地面的距离（限频射线）。 */
	void UpdateGroundDistance(float DeltaTime);

	// ================================================================ 配置：步态

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Gait")
	FActGaitSettingsSet GaitSettings;

	/** MaxWalkSpeed 跟随步态切换的平滑时间（秒）。防止跑步切走路时速度被瞬间钳住。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Gait", Meta = (ClampMin = 0.0f, ForceUnits = "s"))
	float GaitTransitionTime = 0.2f;

	/** 蹲行速度相对当前步态的比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Gait", Meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float CrouchSpeedScale = 0.3f;

	// ================================================================ 配置：旋转

	/** 当前旋转权威。运行时请用 SetRotationAuthority，它同时会重置曲线基准。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation")
	EACTRotationAuthority RotationAuthority = EACTRotationAuthority::MovementComponent;

	/**
	 * 朝移动方向时的角速度（deg/s），会写进 RotationRate.Yaw。
	 * 运行时想改速度直接改 RotationRate.Yaw，不要再加第二套字段。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float VelocityDirectionInterpSpeed = 540.0f;

	/** 朝相机方向时的角速度（deg/s）。战斗/瞄准需要更快。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float LookingDirectionInterpSpeed = 1080.0f;

	/**
	 * 速度低于此值（cm/s）时不再拿速度方向当旋转目标，避免静止时抖动。
	 * 只在不朝相机方向时有意义。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.0f, ForceUnits = "cm/s"))
	float MinSpeedToOrient = 5.0f;

	/** 曲线模式下多久没收到新样本就自动回 MovementComponent（秒）。兜住"ABP 忘了 Clear"。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.05f, ForceUnits = "s"))
	float AnimationYawTimeout = 0.5f;

	// ================================================================ 配置：地面

	/** 落地缓冲时长（秒），决定 Land 动画能播多久。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Ground", Meta = (ClampMin = 0.0f, ForceUnits = "s"))
	float LandDuration = 0.25f;

	/** 空中地面探测的最大距离（cm）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Ground", Meta = (ClampMin = 0.0f, ForceUnits = "cm"))
	float GroundTraceDistance = 800.0f;

	/** 地面探测间隔（秒）。10Hz 足够，脚底 IK 用的是 Foot Placement 节点而非这个值。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Ground", Meta = (ClampMin = 0.01f, ForceUnits = "s"))
	float GroundTraceInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Ground")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// ================================================================ 运行时状态

	UPROPERTY(Transient, BlueprintReadOnly, Category = "ACT|State")
	EACTGait Gait = EACTGait::Run;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "ACT|State")
	EACTRotationMode RotationMode = EACTRotationMode::VelocityDirection;

	/** 动画层读取入口。ABP 用 Property Access 直接读这个结构体的成员
	 *  （路径：CharMoveComp.LocomotionState.Xxx），所以必须是 BlueprintReadOnly ——
	 *  否则编辑器里根本选不到这条路径。
	 *  C++ 侧要零拷贝就用 GetLocomotionState()（不加 UFUNCTION，UFUNCTION 不能返回引用）。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "ACT|State")
	FActLocomotionState LocomotionState;

	/** false 时引擎不再自动刷新朝向（MovementComponent 模式）。 */
	UPROPERTY(Transient)
	bool bAutoUpdateYaw = true;

	/**
	 * 目标偏航角（度）。**持久状态，不随输入归零而清空**。
	 *
	 * 这是点按能转到位的根本：按一下 A 目标就是 90°，松手后输入和速度都没了，
	 * 但目标留着，PhysicsRotation 会继续朝它转直到对准。
	 * 它和"当前朝向"分离，也是平滑转向能成立的前提。
	 */
	UPROPERTY(Transient)
	float TargetYawAngle = 0.0f;

	/** 进入曲线模式那一刻的朝向。曲线的偏移量相对它解释。 */
	UPROPERTY(Transient)
	float AnimDrivenEntryYaw = 0.0f;

	/** 当前应用的绝对朝向（曲线模式）。= EntryYaw + 最新偏移。 */
	UPROPERTY(Transient)
	float AnimDrivenYaw = 0.0f;

	/** 距离上次收到新样本的时间（秒）。超过 AnimationYawTimeout 就退回引擎模式。 */
	UPROPERTY(Transient)
	float AnimYawSilenceTimer = 0.0f;

	/** 已消费到的曲线样本序号，用来判断"这一帧有没有新数据"。 */
	UPROPERTY(Transient)
	uint32 LastConsumedYawSeq = 0;

	UPROPERTY(Transient)
	TObjectPtr<UACTAnimInstance> CachedAnimInstance;

	/** 逐帧插值的最大速度，MaxWalkSpeed 的真实来源。 */
	UPROPERTY(Transient)
	float SmoothedMaxSpeed = 0.0f;

	UPROPERTY(Transient)
	float LandingTimer = 0.0f;

	UPROPERTY(Transient)
	float GroundTraceAccumulator = 0.0f;

	UPROPERTY(Transient)
	float CachedGroundDistance = 0.0f;

	UPROPERTY(Transient)
	float SmoothedAccelerationMagnitude = 0.0f;

	UPROPERTY(Transient)
	float SmoothedBrakeAmount = 0.0f;
};
