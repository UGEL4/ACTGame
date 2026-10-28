// ACT 移动组件 —— 胶囊体位移与朝向的唯一权威
//
// ================================================================ 它做什么
//   1. 位移：输入 → 加速度/减速度 → 速度 → 移动。所有速度参数都是"时间"换算出来的。
//   2. 朝向：只有一个目标角和一个当前角，按固定角速度平滑过去（或瞬时对齐）。
//   3. 快照：把动画层需要、而动画层算不出来的量组装成 FActLocomotionState。
//
// ================================================================ 它不做什么（重要）
//   1. **不判定转身**：没有 pivot 请求、没有大角度提速、没有速度折扣、没有冷却。
//      "什么时候播转身/折返动画、转多少度"是动画层的决定，见
//      Docs/Turn-Pivot-CurveDriven-Design.md（§4 曲线契约 / §8.5 瞬转折返）。
//      组件只提供两个原语：SetTargetYawAngle()（含 bInstant）和 SetRotationInterpSpeed()。
//   2. **不碰动画资产**：不认识 Montage，不认识曲线。动画层通过 Property Access 只读快照。
//   3. **不做网络预测**：没有 FSavedMove、没有 Desired*/实际值双状态。
//
// ================================================================ 两条容易踩的坑（已处理，别改回去）
//   1. 旋转权威唯一：bOrientRotationToMovement / bUseControllerDesiredRotation / Character 的
//      bUseControllerRotation* 全部关闭，否则 PhysicsRotation 会和引擎抢朝向。
//   2. BrakingFrictionFactor 必须为 0：否则 ApplyVelocityBraking 里的指数摩擦项会盖掉
//      恒定减速度，TimeToStop 形同虚设（见 .cpp 构造函数注释）。
//
// 结构对照 ALS-Refactored 的 CMC，但剥离了网络同步、转身动作、蒙太奇与曲线驱动。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Locomotion/ACTLocomotionTypes.h"
#include "ACTCharacterMovementComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogACTMovement, Log, All);

UCLASS()
class ACTGAME_API UACTCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UACTCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 朝向的唯一写入口。引擎在 PerformMovement 末尾调用，蒙太奇 root motion 期间自动跳过。 */
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

	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetRotationMode(EACTRotationMode NewMode);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	EACTRotationMode GetRotationMode() const { return RotationMode; }

	// ================================================================ 朝向

	/**
	 * 设置目标偏航角（度）。这是改变朝向的唯一入口。
	 * @param bInstant true = 下一帧直接对齐（折返、翻滚、瞬移、攻击起手），否则按 RotationInterpSpeed 插值。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetTargetYawAngle(float NewTargetYaw, bool bInstant = false);

	/**
	 * 是否允许每帧自动刷新目标偏航角（默认 true）。
	 * 关掉后 SetTargetYawAngle 设的值会保持住 —— 攻击锁定朝向、折返窗口内都必须关，
	 * 否则 UpdateDesiredYaw 每帧会把手动设的角度覆盖回输入/速度方向。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetAutoYawUpdate(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	float GetTargetYawAngle() const { return TargetYawAngle; }

	/** 临时改变转向速度（deg/s），传 0 冻结旋转。用于攻击期间锁朝向、处决动画等。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetRotationInterpSpeed(float NewSpeed);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	float GetRotationInterpSpeed() const;

	// ================================================================ 动画层数据

	/** 动画层唯一数据源。
	 *  注意：不加 UFUNCTION —— UFUNCTION 不能返回引用，这里必须是零拷贝的 C++ 访问。 */
	const FActLocomotionState& GetLocomotionState() const { return LocomotionState; }

	const FActGaitSettings& GetGaitSettings(EACTGait InGait) const { return GaitSettings.Get(InGait); }

protected:
	// ================================================================ 内部更新

	/** 根据 RotationMode 推导本帧的目标偏航角。必须在 Super::TickComponent 之前调用。 */
	void UpdateDesiredYaw();

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

	// ================================================================ 配置：朝向

	/** 朝速度方向时的转向速度（deg/s）。540 ≈ 0.33 秒转 180°。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float VelocityDirectionInterpSpeed = 540.0f;

	/** 朝相机方向时的转向速度（deg/s）。战斗/瞄准需要更快。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float LookingDirectionInterpSpeed = 1080.0f;

	/** 速度低于此值（cm/s）时不再更新目标偏航角，避免静止时抖动。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.0f, ForceUnits = "cm/s"))
	float MinSpeedToOrient = 5.0f;

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

	/** 目标偏航角（度）。 */
	UPROPERTY(Transient)
	float TargetYawAngle = 0.0f;

	/** 实际偏航角（度）。与 TargetYawAngle 分离，是这个系统能平滑转向的前提。 */
	UPROPERTY(Transient)
	float CurrentYawAngle = 0.0f;

	UPROPERTY(Transient)
	bool bInstantYaw = false;

	/** false 时 UpdateDesiredYaw 不再工作，目标角由外部完全控制（攻击锁定 / 折返窗口）。 */
	UPROPERTY(Transient)
	bool bAutoUpdateYaw = true;

	/** 逐帧插值的最大速度，MaxWalkSpeed 的真实来源。 */
	UPROPERTY(Transient)
	float SmoothedMaxSpeed = 0.0f;

	/** SetRotationInterpSpeed 的覆盖值，负数表示用 RotationMode 的默认值。 */
	UPROPERTY(Transient)
	float RotationInterpSpeedOverride = -1.0f;

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
