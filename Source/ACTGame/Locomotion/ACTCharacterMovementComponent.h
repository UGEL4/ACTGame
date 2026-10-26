// ACT 移动组件 —— 胶囊体位移的唯一权威
//
// 设计原则（对照 ALS-Refactored，但已剥离网络同步相关内容）：
//   1. **旋转单一权威**：关掉 bOrientRotationToMovement / bUseControllerDesiredRotation / Character 的
//      bUseControllerRotation*，所有朝向只能经 SetTargetYawAngle() 或 SetRotationMode() 改变。
//      实现落在 PhysicsRotation()（引擎在 PerformMovement 末尾调用，蒙太奇 root motion 期间自动跳过）。
//   2. **参数换算而非硬编码**：加速度 = MaxSpeed / TimeToMaxSpeed，减速度 = MaxSpeed / TimeToStop。
//      因此 BrakingFrictionFactor 必须置 0，否则 ApplyVelocityBraking 里的指数摩擦项会盖掉恒定减速度，
//      TimeToStop 形同虚设（这是最容易踩的坑，见 .cpp 构造函数注释）。
//   3. **动画层零推导**：所有需要给动画层的值都在 UpdateLocomotionState() 里算完，
//      动画蓝图只读 FActLocomotionState，不 Cast、不读曲线。
//   4. 不做网络预测：没有 FSavedMove、没有 Desired*/实际值双状态。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Locomotion/ACTLocomotionTypes.h"
#include "ACTCharacterMovementComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogACTMovement, Log, All);

/**
 * 请求播放一次 pivot（急转身）动画。参数是剩余转向角（度，正 = 向左）。
 *
 * 用事件而不是每帧查询的 bool：pivot 是一次性离散行为，必须"触发一次、播一次"。
 * 用 bool 让外部轮询，很容易在状态机里重复读到 true 而连播。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FACTPivotRequested, float, TurnAngle);

UCLASS()
class ACTGAME_API UACTCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UACTCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 旋转的唯一执行点。引擎在 PerformMovement 末尾调用，蒙太奇 root motion 期间自动跳过。 */
	virtual void PhysicsRotation(float DeltaTime) override;

	/** 落地钩子：置 Landing 计时，动画层据此播落地缓冲。 */
	virtual void ProcessLanded(const FHitResult& Hit, float RemainingTime, int32 Iterations) override;

	// ================================================================ 对外接口

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

	/**
	 * 设置目标偏航角（度）。这是改变朝向的唯一入口。
	 * @param bInstant true = 下一帧直接对齐（翻滚、瞬移、攻击起手等），否则按 RotationInterpSpeed 插值。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetTargetYawAngle(float NewTargetYaw, bool bInstant = false);

	/**
	 * 是否允许每帧自动刷新目标偏航角（默认 true）。
	 * 关掉后 SetTargetYawAngle 设的值会保持住 —— 攻击时"锁定朝向敌人"必须关，
	 * 否则 UpdateDesiredYaw 每帧会把手动设的角度覆盖回速度/相机方向。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetAutoYawUpdate(bool bEnable);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	float GetTargetYawAngle() const { return TargetYawAngle; }

	/** 当前实际偏航角（度）。与 TargetYawAngle 的差就是动画层 Orientation Warping 的输入。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	float GetCurrentYawAngle() const { return CurrentYawAngle; }

	/** 临时改变转向速度（deg/s），传 0 冻结旋转。用于攻击期间锁朝向、处决动画等。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetRotationInterpSpeed(float NewSpeed);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	float GetRotationInterpSpeed() const;

	/** 动画层唯一数据源。
	 *  注意：不加 UFUNCTION —— UFUNCTION 不能返回引用，这里必须是零拷贝的 C++ 访问。 */
	const FActLocomotionState& GetLocomotionState() const { return LocomotionState; }

	const FActGaitSettings& GetGaitSettings(EACTGait InGait) const { return GaitSettings.Get(InGait); }

	/** 当前是否处于落地缓冲（动画层可直接用 LocomotionState.bLanding）。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	bool IsLanding() const { return LandingTimer > 0.0f; }

	// ================================================================ 转向

	/** 请求一次 pivot 动画。目前只有移动组件自己发（见 UpdateTurnState），保留 BP 可绑定。 */
	UPROPERTY(BlueprintAssignable, Category = "ACT|Locomotion")
	FACTPivotRequested OnPivotRequested;

	/** 还需要转多少度（有符号，正 = 向左）。= 目标朝向 − 当前朝向。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	float GetTurnAngle() const;

	/** 是否处于"大角度转向"（|TurnAngle| > SharpTurnAngleThreshold）。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	bool IsSharpTurning() const;

	/** 当前转向导致的速度折扣（0~1）。1 = 不受影响。调试用。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	float GetTurnSpeedScale() const { return TurnSpeedScale; }

protected:
	// ================================================================ 内部更新

	/** 根据 RotationMode 推导本帧的目标偏航角。必须在 Super::TickComponent 之前调用。 */
	void UpdateDesiredYaw(float DeltaTime);

	/** 转向分级：稳定性计时、pivot 判定、速度折扣。必须在 UpdateDesiredYaw 之后、
	 *  UpdateGaitParameters 之前调用（后者要用 TurnSpeedScale）。 */
	void UpdateTurnState(float DeltaTime);

	/** 把 GaitSettings 里的时间换算成 MaxAcceleration / BrakingDecelerationWalking。 */
	void UpdateGaitParameters(float DeltaTime);

	/** 组装给动画层的只读快照。在 Super::TickComponent 之后调用，读到的是本帧移动结果。 */
	void UpdateLocomotionState(float DeltaTime);

	/** 空中时探测脚底到地面的距离（限频射线）。 */
	void UpdateGroundDistance(float DeltaTime);

	// ================================================================ 配置

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Gait")
	FActGaitSettingsSet GaitSettings;

	/** MaxWalkSpeed 跟随步态切换的平滑时间（秒）。防止跑步切走路时速度被瞬间钳住。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Gait", Meta = (ClampMin = 0.0f, ForceUnits = "s"))
	float GaitTransitionTime = 0.2f;

	/** 蹲行速度相对当前步态的比例。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Gait", Meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float CrouchSpeedScale = 0.3f;

	/** 朝速度方向时的转向速度（deg/s）。540 ≈ 0.33 秒转 180°。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float VelocityDirectionInterpSpeed = 540.0f;

	/** 朝相机方向时的转向速度（deg/s）。战斗/瞄准需要更快。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float LookingDirectionInterpSpeed = 1080.0f;

	/** 速度低于此值（cm/s）时不再更新目标偏航角，避免静止时抖动。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Rotation", Meta = (ClampMin = 0.0f, ForceUnits = "cm/s"))
	float MinSpeedToOrient = 5.0f;

	// ---------------------------------------------------------------- 大角度转向 / 180 反打

	/** 超过这个角度开始提速转向（度）。小角度用基础速度，避免直线奔跑时左右微调过冲。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Turn", Meta = (ClampMin = 0.0f, ClampMax = 180.0f, ForceUnits = "deg"))
	float SharpTurnAngleThreshold = 45.0f;

	/** 大角度时的旋转速度（deg/s）。180° 用 1080 约 0.17 秒转完；540 要 0.33 秒，明显"舍不得转身"。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Turn", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float SharpTurnInterpSpeed = 1080.0f;

	/** |TurnAngle| 超过这个角度才考虑请求 pivot 动画（度）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Turn", Meta = (ClampMin = 0.0f, ClampMax = 180.0f, ForceUnits = "deg"))
	float PivotAngleThreshold = 140.0f;

	/** 速度低于此值（cm/s）不播 pivot —— 慢速转身用上面的提速旋转就够了，不需要专用 clip。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Turn", Meta = (ClampMin = 0.0f, ForceUnits = "cm/s"))
	float PivotMinSpeed = 200.0f;

	/**
	 * 输入方向变化率低于此值（deg/s）才认为玩家"已经选定"新方向。
	 * 这是区分「摇杆绕圈」和「真的反打」的唯一可靠信号：
	 * 绕圈时角速度持续偏高，反打是跳一下然后立刻归零。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Turn", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float TurnStableRate = 120.0f;

	/** 输入方向稳定多久之后才允许触发 pivot（秒）。防止瞬时的方向抖动就播动画。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Turn", Meta = (ClampMin = 0.0f, ForceUnits = "s"))
	float TurnStableDwell = 0.1f;

	/** 两次 pivot 的最小间隔（秒）。必须 >= pivot 蒙太奇时长，否则连续快速转身会变成蒙太奇连播。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Turn", Meta = (ClampMin = 0.0f, ForceUnits = "s"))
	float PivotCooldown = 0.9f;

	/** 开始压低速度的转向角度（度）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Turn", Meta = (ClampMin = 0.0f, ClampMax = 180.0f, ForceUnits = "deg"))
	float TurnBrakeStartAngle = 60.0f;

	/** 180° 反向时速度保留的比例。这个值决定转身时"掉多少速"，是最影响观感的一个参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ACT|Turn", Meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float TurnBrakeMinScale = 0.15f;

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

	/** false 时 UpdateDesiredYaw 不再工作，目标角由外部完全控制（攻击锁定用）。 */
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

	// ---------------------------------------------------------------- 转向状态

	/** 输入方向的角速度（deg/s，取绝对值并平滑）。用于区分"绕圈"和"反打"。 */
	UPROPERTY(Transient)
	float SmoothedTurnRate = 0.0f;

	UPROPERTY(Transient)
	float PrevInputYaw = 0.0f;

	UPROPERTY(Transient)
	bool bHasPrevInputYaw = false;

	/** 输入方向已经稳定了多久（秒）。超过 TurnStableDwell 才允许 pivot。 */
	UPROPERTY(Transient)
	float TurnStableTimer = 0.0f;

	/** pivot 冷却剩余时间（秒）。 */
	UPROPERTY(Transient)
	float PivotCooldownTimer = 0.0f;

	/** 本帧的转向速度折扣，乘在 MaxWalkSpeed 上。 */
	UPROPERTY(Transient)
	float TurnSpeedScale = 1.0f;
};
