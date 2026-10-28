#include "Locomotion/ACTCharacterMovementComponent.h"

#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY(LogACTMovement);

namespace
{
	/** 平滑系数（1/s）。越大越跟手，越小越迟钝。 */
	constexpr float AccelerationSmoothingRate = 14.0f;
	constexpr float BrakeSmoothingRate = 10.0f;

	/** 判定"在减速"的速度变化阈值（cm/s²）。低于它认为是噪声。 */
	constexpr float BrakingAccelThreshold = 20.0f;
}

UACTCharacterMovementComponent::UACTCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// ---------------------------------------------------------------- 旋转
	// 关掉引擎的两条自动旋转路径，让 PhysicsRotation() 成为唯一权威。
	// 对应 ALS 的做法：CMC 的 RotationRate / bOrientRotationToMovement 全关，Character 的
	// bUseControllerRotation* 也关（在 BeginPlay 里补，构造函数拿不到 CharacterOwner）。
	bOrientRotationToMovement = false;
	bUseControllerDesiredRotation = false;
	RotationRate = FRotator::ZeroRotator;

	// 蒙太奇 root motion 期间不要插一脚旋转，交给动画。这也让 PhysicsRotation 被引擎跳过。
	bAllowPhysicsRotationDuringAnimRootMotion = false;

	// ---------------------------------------------------------------- 减速（关键）
	// ApplyVelocityBraking() 的实际公式是：Velocity += (-Friction * Velocity + RevAccel) * dt
	// 其中 RevAccel 才是 BrakingDeceleration。默认 GroundFriction=8、BrakingFrictionFactor=2，
	// 指数项的时间常数约 1/16 秒，会在 0.06 秒内把速度吃光 —— 这样 BrakingDecelerationWalking
	// 完全不起作用，TimeToStop 填多少都一样。
	// 置 0 后 bZeroFriction 成立，只剩恒定减速度，TimeToStop 才是可预期的。
	// 注意：这个乘数只影响"刹车"，不影响 CalcVelocity 里用于转向响应的 GroundFriction。
	BrakingFrictionFactor = 0.0f;

	// ---------------------------------------------------------------- 其它
	// 空中不要额外的横向摩擦，跳跃弧线由输入决定（否则松开摇杆会在空中急停）。
	FallingLateralFriction = 0.0f;

	// 默认值（BeginPlay 会按 GaitSettings 重新算，这里只是让 CDO 在编辑器里好看一点）。
	GaitSettings.Walk.BlendSpaceSpeed = 150.0f;
	GaitSettings.Walk.MaxSpeed = 150.0f;
	GaitSettings.Walk.TimeToMaxSpeed = 0.35f;
	GaitSettings.Walk.TimeToStop = 0.25f;

	GaitSettings.Run.BlendSpaceSpeed = 500.0f;
	GaitSettings.Run.MaxSpeed = 500.0f;
	GaitSettings.Run.TimeToMaxSpeed = 0.35f;
	GaitSettings.Run.TimeToStop = 0.3f;

	GaitSettings.Sprint.BlendSpaceSpeed = 800.0f;
	GaitSettings.Sprint.MaxSpeed = 800.0f;
	GaitSettings.Sprint.TimeToMaxSpeed = 0.55f;
	GaitSettings.Sprint.TimeToStop = 0.45f;
}

void UACTCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (CharacterOwner)
	{
		// ACharacter 默认 bUseControllerRotationYaw = true，会让角色直接吸附控制器朝向，
		// 与我们的平滑转向打架（表现为转向瞬间完成、动画跟不上）。必须关。
		CharacterOwner->bUseControllerRotationPitch = false;
		CharacterOwner->bUseControllerRotationYaw = false;
		CharacterOwner->bUseControllerRotationRoll = false;

		CurrentYawAngle = CharacterOwner->GetActorRotation().Yaw;
		TargetYawAngle = CurrentYawAngle;
		LocomotionState.YawOffset = 0.0f;

		// 初始化基准速度，否则第一帧 (NewSpeed - 0) / dt 会产生一个巨大的假加速度，
		// Lean 会瞬间抽一下。
		LocomotionState.Speed = Velocity.Size2D();
		LocomotionState.VerticalSpeed = Velocity.Z;
	}

	// 初始化速度参数，避免第一帧 SmoothedMaxSpeed 从 0 爬起来（角色会先"起步慢半拍"）。
	const FActGaitSettings& Settings = GetGaitSettings(Gait);
	SmoothedMaxSpeed = Settings.MaxSpeed;
	MaxWalkSpeed = SmoothedMaxSpeed;
	MaxWalkSpeedCrouched = SmoothedMaxSpeed * CrouchSpeedScale;
	MaxAcceleration = Settings.MaxSpeed / FMath::Max(Settings.TimeToMaxSpeed, 0.01f);
	BrakingDecelerationWalking = Settings.MaxSpeed / FMath::Max(Settings.TimeToStop, 0.01f);

	CachedGroundDistance = 0.0f;
}

void UACTCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// 先定方向、再定速度参数，最后交给引擎做位移。顺序不能换：
	// UpdateGaitParameters 会写 MaxWalkSpeed / MaxAcceleration / BrakingDeceleration，
	// 而这三个值必须在 Super::TickComponent（内部走 PerformMovement → CalcVelocity）之前就位。
	UpdateDesiredYaw();
	UpdateGaitParameters(DeltaTime);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 位移完成后才统计加速/刹车/落地，动画层读到的才是本帧真实结果。
	UpdateGroundDistance(DeltaTime);
	UpdateLocomotionState(DeltaTime);
}

// ==================================================================== 旋转

void UACTCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (!HasValidData() || !UpdatedComponent)
	{
		return;
	}

	// 翻滚等蒙太奇自己带旋转，此时不插手。但缓存必须跟着走：否则 RM 转完之后
	// CurrentYawAngle 还是旧值，下一帧 FixedTurn 会从旧角度出发把角色硬拽回去
	// （表现为翻滚结束时"回弹一下"）。
	if (HasAnimRootMotion())
	{
		CurrentYawAngle = CharacterOwner->GetActorRotation().Yaw;
		return;
	}

	// 同上，只是来源更广：传送、SetActorRotation、蓝图直接改朝向都会让缓存变脏。
	// 阈值 1° 是为了不把 PhysicsRotation 自己的浮点误差当成漂移。
	{
		const float ActorYaw = CharacterOwner->GetActorRotation().Yaw;
		if (FMath::Abs(FRotator::NormalizeAxis(ActorYaw - CurrentYawAngle)) > 1.0f)
		{
			CurrentYawAngle = ActorYaw;
		}
	}

	// 瞬时对齐：折返、翻滚起手、攻击起手都走这条。只生效一帧。
	if (bInstantYaw)
	{
		bInstantYaw = false;
		CurrentYawAngle = TargetYawAngle;
		MoveUpdatedComponent(FVector::ZeroVector, FQuat(FRotator(0.0f, CurrentYawAngle, 0.0f)), false);
		return;
	}

	const float InterpSpeed = GetRotationInterpSpeed();
	if (InterpSpeed <= 0.0f)
	{
		return;
	}

	// FixedTurn 保证"不超过目标"，避免小角度时来回过冲。
	const float NewYaw = FMath::FixedTurn(CurrentYawAngle, TargetYawAngle, InterpSpeed * DeltaTime);
	if (FMath::IsNearlyEqual(NewYaw, CurrentYawAngle, 0.01f))
	{
		return;
	}

	CurrentYawAngle = FRotator::NormalizeAxis(NewYaw);
	MoveUpdatedComponent(FVector::ZeroVector, FQuat(FRotator(0.0f, CurrentYawAngle, 0.0f)), false);
}

void UACTCharacterMovementComponent::UpdateDesiredYaw()
{
	if (!CharacterOwner || !bAutoUpdateYaw)
	{
		return;
	}

	if (RotationMode == EACTRotationMode::LookingDirection)
	{
		if (const AController* OwningController = CharacterOwner->GetController())
		{
			TargetYawAngle = OwningController->GetControlRotation().Yaw;
		}
		return;
	}

	// ---------------------------------------------------------------- 目标方向
	// VelocityDirection 模式：用「输入意图」而不是速度方向。两个理由：
	//
	// 1. 速度方向在反向瞬间会穿过 0，那一刻方向纯是数值噪声，目标角会乱蹦。
	// 2. 更要命的：玩家把摇杆从后推到前，意图立刻变了，但速度要 0.6 秒才真正掉头
	//    （TimeToMaxSpeed 0.35 + TimeToStop 0.3）。用速度做目标，角色必须"等速度转过来"
	//    才开始转身 —— 这就是转身慢半拍、看起来像被拖着的根本原因。
	//
	// GetLastMovementInputVector() 是上一帧被 ConsumeMovementInputVector 消费掉的输入（世界空间），
	// 引擎注释明确写了"动画更新应该用它"，且在 TickComponent 前后读都有效。
	const FVector InputVector = CharacterOwner->GetLastMovementInputVector();

	if (InputVector.SizeSquared2D() > 0.01f)
	{
		TargetYawAngle = FRotator::NormalizeAxis(InputVector.ToOrientationRotator().Yaw);
	}
	else if (Velocity.SizeSquared2D() > FMath::Square(MinSpeedToOrient))
	{
		// 松手滑行阶段没输入意图，保持"往哪滑就往哪"，不要突然回头。
		TargetYawAngle = FRotator::NormalizeAxis(Velocity.ToOrientationRotator().Yaw);
	}
	// 两者都没有：保持上一次的目标角（静止时不要抖）。
}

void UACTCharacterMovementComponent::SetTargetYawAngle(float NewTargetYaw, bool bInstant)
{
	TargetYawAngle = FRotator::NormalizeAxis(NewTargetYaw);
	bInstantYaw = bInstant;
}

void UACTCharacterMovementComponent::SetRotationMode(EACTRotationMode NewMode)
{
	RotationMode = NewMode;
}

void UACTCharacterMovementComponent::SetAutoYawUpdate(bool bEnable)
{
	bAutoUpdateYaw = bEnable;
}

void UACTCharacterMovementComponent::SetRotationInterpSpeed(float NewSpeed)
{
	// 负数 = 恢复 RotationMode 默认值。不能钳到 0 —— 0 的语义是「冻结旋转」，
	// 钳过去会让攻击/折返结束后无法恢复默认转向速度。
	RotationInterpSpeedOverride = NewSpeed < 0.0f ? -1.0f : NewSpeed;
}

float UACTCharacterMovementComponent::GetRotationInterpSpeed() const
{
	if (RotationInterpSpeedOverride >= 0.0f)
	{
		return RotationInterpSpeedOverride;
	}

	return RotationMode == EACTRotationMode::LookingDirection
		? LookingDirectionInterpSpeed
		: VelocityDirectionInterpSpeed;
}

// ==================================================================== 步态 / 站姿

void UACTCharacterMovementComponent::SetGait(EACTGait NewGait)
{
	if (Gait == NewGait)
	{
		return;
	}

	Gait = NewGait;
	UE_LOG(LogACTMovement, Verbose, TEXT("[%s] Gait -> %s"),
		*GetNameSafe(CharacterOwner), *StaticEnum<EACTGait>()->GetNameStringByValue(static_cast<int64>(Gait)));
}

void UACTCharacterMovementComponent::SetStance(EACTStance NewStance)
{
	if (!CharacterOwner)
	{
		return;
	}

	if (NewStance == EACTStance::Crouch)
	{
		CharacterOwner->Crouch();
	}
	else
	{
		CharacterOwner->UnCrouch();
	}
}

EACTStance UACTCharacterMovementComponent::GetStance() const
{
	return (CharacterOwner && CharacterOwner->bIsCrouched) ? EACTStance::Crouch : EACTStance::Stand;
}

void UACTCharacterMovementComponent::UpdateGaitParameters(float DeltaTime)
{
	const FActGaitSettings& Settings = GetGaitSettings(Gait);

	// 不直接把 MaxWalkSpeed 设成目标值：从 Run(500) 切到 Walk(150) 的瞬间，
	// CalcVelocity 里的 IsExceedingMaxSpeed 会把速度硬钳到 150，看起来像急刹。
	// 插值过去，加速/减速仍然由下面的 MaxAcceleration / BrakingDeceleration 控制。
	if (SmoothedMaxSpeed <= 0.0f)
	{
		// 首帧 / 刚构造：直接对齐，避免角色从 0 开始"热身"。
		SmoothedMaxSpeed = Settings.MaxSpeed;
	}
	else if (DeltaTime > 0.0f && GaitTransitionTime > KINDA_SMALL_NUMBER)
	{
		SmoothedMaxSpeed = FMath::FInterpTo(SmoothedMaxSpeed, Settings.MaxSpeed, DeltaTime, 1.0f / GaitTransitionTime);
	}

	MaxWalkSpeed = SmoothedMaxSpeed;
	MaxWalkSpeedCrouched = SmoothedMaxSpeed * FMath::Clamp(CrouchSpeedScale, 0.0f, 1.0f);

	// 核心换算：a = Δv / t
	MaxAcceleration = Settings.MaxSpeed / FMath::Max(Settings.TimeToMaxSpeed, 0.01f);
	BrakingDecelerationWalking = Settings.MaxSpeed / FMath::Max(Settings.TimeToStop, 0.01f);
}

// ==================================================================== 落地 / 地面

void UACTCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float RemainingTime, int32 Iterations)
{
	Super::ProcessLanded(Hit, RemainingTime, Iterations);

	LandingTimer = LandDuration;
}

void UACTCharacterMovementComponent::UpdateGroundDistance(float DeltaTime)
{
	if (IsMovingOnGround())
	{
		CachedGroundDistance = 0.0f;
		return;
	}

	GroundTraceAccumulator += DeltaTime;
	if (GroundTraceAccumulator < GroundTraceInterval)
	{
		return;
	}
	GroundTraceAccumulator = 0.0f;

	const UCapsuleComponent* Capsule = CharacterOwner ? CharacterOwner->GetCapsuleComponent() : nullptr;
	if (!Capsule)
	{
		return;
	}

	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector FeetLocation = Capsule->GetComponentLocation() - FVector(0.0f, 0.0f, HalfHeight);

	FCollisionQueryParams QueryParams(FName(TEXT("ACTGroundTrace")), false, CharacterOwner);
	FHitResult Hit;

	const FVector TraceEnd = FeetLocation - FVector(0.0f, 0.0f, GroundTraceDistance);
	if (GetWorld()->LineTraceSingleByChannel(Hit, FeetLocation, TraceEnd, GroundTraceChannel, QueryParams))
	{
		CachedGroundDistance = FMath::Max(0.0f, FeetLocation.Z - Hit.ImpactPoint.Z);
	}
	else
	{
		// 探测不到地面时给一个大值，动画层据此认为"还在高处"，不会提前播落地预备。
		CachedGroundDistance = GroundTraceDistance;
	}
}

// ==================================================================== 动画层快照

void UACTCharacterMovementComponent::UpdateLocomotionState(float DeltaTime)
{
	if (!CharacterOwner)
	{
		return;
	}

	if (LandingTimer > 0.0f)
	{
		LandingTimer = FMath::Max(0.0f, LandingTimer - DeltaTime);
	}

	// ---------------------------------------------------------------- 速度
	const float NewSpeed = Velocity.Size2D();
	const float OldSpeed = LocomotionState.Speed;

	LocomotionState.Speed = NewSpeed;
	LocomotionState.VerticalSpeed = Velocity.Z;

	if (NewSpeed > 1.0f)
	{
		const float VelocityYaw = Velocity.ToOrientationRotator().Yaw;
		LocomotionState.VelocityYawOffset =
			FRotator::NormalizeAxis(VelocityYaw - CharacterOwner->GetActorRotation().Yaw);
	}
	else
	{
		LocomotionState.VelocityYawOffset = 0.0f;
	}

	// 朝向修正角：Orientation Warping 的输入。这是 ALS 用 RotationYawOffsetCurve 干的事，
	// 这里由"目标角 - 实际角"直接得出，不需要美术 K 曲线。
	//
	// 注意：折返（瞬转）期间 VelocityYawOffset 会接近 180°，动画层必须把 Orientation Warping
	// 的权重压到 0 —— 那是"倒着滑"，不是"要转身"，节点会把腿扭到反方向。
	LocomotionState.YawOffset = FRotator::NormalizeAxis(TargetYawAngle - CharacterOwner->GetActorRotation().Yaw);

	// ---------------------------------------------------------------- 输入
	// 用 GetLastMovementInputVector（上一帧被消费的移动输入），而不是 Acceleration：
	// Acceleration 在 PerformMovement 之后可能已被清空，读它拿到的是不可靠的值。
	const FVector InputVector = CharacterOwner->GetLastMovementInputVector();
	if (!InputVector.IsNearlyZero())
	{
		const FVector Forward = CharacterOwner->GetActorForwardVector().GetSafeNormal2D();
		const FVector Right = CharacterOwner->GetActorRightVector().GetSafeNormal2D();

		FVector2D LocalInput(FVector::DotProduct(InputVector, Right), FVector::DotProduct(InputVector, Forward));
		if (LocalInput.SizeSquared() > 1.0f)
		{
			LocalInput.Normalize();
		}
		LocomotionState.MoveInput = LocalInput;
	}
	else
	{
		LocomotionState.MoveInput = FVector2D::ZeroVector;
	}

	// ---------------------------------------------------------------- 加速 / 刹车
	// 用速度的实际变化率，而不是 GetCurrentAcceleration()：后者是"期望加速度"，
	// 与实际感受（有摩擦、有斜坡、被墙挡住）不一致，Lean 会跟着抖。
	const float SpeedDeltaPerSecond = DeltaTime > KINDA_SMALL_NUMBER ? (NewSpeed - OldSpeed) / DeltaTime : 0.0f;
	const float AbsSpeedDelta = FMath::Abs(SpeedDeltaPerSecond);

	SmoothedAccelerationMagnitude = FMath::FInterpTo(SmoothedAccelerationMagnitude, AbsSpeedDelta, DeltaTime, AccelerationSmoothingRate);

	const bool bBraking = SpeedDeltaPerSecond < -BrakingAccelThreshold;
	SmoothedBrakeAmount = FMath::FInterpTo(SmoothedBrakeAmount, bBraking ? 1.0f : 0.0f, DeltaTime, BrakeSmoothingRate);

	LocomotionState.AccelerationMagnitude = SmoothedAccelerationMagnitude;
	LocomotionState.BrakeAmount = SmoothedBrakeAmount;

	// ---------------------------------------------------------------- 状态机
	if (IsMovingOnGround())
	{
		LocomotionState.Mode = LandingTimer > 0.0f ? EACTLocomotionMode::Landing : EACTLocomotionMode::Grounded;
	}
	else
	{
		LocomotionState.Mode = Velocity.Z > 0.0f ? EACTLocomotionMode::Jumping : EACTLocomotionMode::Falling;
	}

	LocomotionState.bLanding = (LocomotionState.Mode == EACTLocomotionMode::Landing);
	LocomotionState.Stance = GetStance();
	LocomotionState.Gait = Gait;
	LocomotionState.RotationMode = RotationMode;

	// ---------------------------------------------------------------- 地面
	LocomotionState.DistanceToGround = IsMovingOnGround()
		? FMath::Max(0.0f, CurrentFloor.FloorDist)
		: CachedGroundDistance;

	// TimeToLand 只用重力公式推导，不打射线（动画层要的是"还有多久"，不是"下面是什么"）。
	const float Gravity = FMath::Max(FMath::Abs(GetGravityZ()), 1.0f);
	switch (LocomotionState.Mode)
	{
	case EACTLocomotionMode::Jumping:
		// 上抛阶段：先到顶点再落回同一高度，近似为 2v/g。
		LocomotionState.TimeToLand = FMath::Max(0.0f, 2.0f * Velocity.Z / Gravity);
		break;
	case EACTLocomotionMode::Falling:
		LocomotionState.TimeToLand = FMath::Clamp(FMath::Abs(Velocity.Z) / Gravity, 0.0f, 5.0f);
		break;
	default:
		LocomotionState.TimeToLand = 0.0f;
		break;
	}
}
