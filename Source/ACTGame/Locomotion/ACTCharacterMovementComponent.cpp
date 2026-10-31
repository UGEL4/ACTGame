#include "Locomotion/ACTCharacterMovementComponent.h"

#include "Animation/ACTAnimInstance.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

	/** 判定"外部改了朝向"的角度阈值（度）。低于它认为是浮点误差。 */
	constexpr float ExternalYawDriftThreshold = 1.0f;
}

UACTCharacterMovementComponent::UACTCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// ---------------------------------------------------------------- 旋转
	// 引擎的两条自动旋转路径全部关掉，由本类的 PhysicsRotation 独占。
	//
	// 为什么不用 bOrientRotationToMovement：它的目标只能来自 Acceleration
	// （ComputeOrientToMovementRotation），而玩家点按一下方向键、松手时 Acceleration 立刻归零，
	// 引擎会直接 `return CurrentRotation` —— 一帧最多转 RotationRate.Yaw * dt ≈ 9°，
	// 点按只能转十几度就停住，看起来像"点了没反应"。
	// 我们需要的是"松手后靠惯性继续转完"，这只能自己实现。
	// 代价是放弃引擎顺带处理的 HasCustomGravity / 非竖直朝向 —— 本项目角色恒竖直，用不上。
	bOrientRotationToMovement = false;
	bUseControllerDesiredRotation = false;
	RotationRate = FRotator(0.0f, VelocityDirectionInterpSpeed, 0.0f);

	// 蒙太奇 root motion 期间不要插一脚旋转，交给动画。这也让 PhysicsRotation 被引擎跳过。
	bAllowPhysicsRotationDuringAnimRootMotion = false;

	// ---------------------------------------------------------------- 减速（关键）
	// ApplyVelocityBraking() 的实际公式是：Velocity += (-Friction * Velocity + RevAccel) * dt
	// 其中 RevAccel 才是 BrakingDeceleration。默认 GroundFriction=8、BrakingFrictionFactor=2，
	// 指数项的时间常数约 1/16 秒，会在 0.06 秒内把速度吃光 —— 这样 BrakingDecelerationWalking
	// 完全不起作用，TimeToStop 填多少都一样。
	// 置 0 后 bZeroFriction 成立，只剩恒定减速度，TimeToStop 才是可预期的。
	// 副作用（是好事）：PredictGroundMovementStopLocation 在 FrictionFactor=0 时退化成
	// v²/(2a)，与恒定减速度精确一致；如果改回非 0，预测会系统性偏短约 35%。
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
		// 与 RotationRate 的限速打架（表现为转向瞬间完成、动画跟不上）。必须关。
		CharacterOwner->bUseControllerRotationPitch = false;
		CharacterOwner->bUseControllerRotationYaw = false;
		CharacterOwner->bUseControllerRotationRoll = false;

		LocomotionState.YawOffset = 0.0f;

		// 目标角必须从当前朝向起步，否则第一帧会突然转向世界坐标的 0°。
		TargetYawAngle = FRotator::NormalizeAxis(CharacterOwner->GetActorRotation().Yaw);

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

	// 最后按当前 RotationMode 刷一次角速度（构造函数里拿不到最终的 RotationMode）。
	RefreshRotationRate();
}

void UACTCharacterMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// 先定目标、再定速度参数，最后交给引擎做位移。顺序不能换：
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

	// 翻滚等蒙太奇自带旋转，此时谁都别插手。
	// 引擎那边靠 bAllowPhysicsRotationDuringAnimRootMotion 也会跳过，但曲线模式是我们自己的
	// 代码路径，必须在这里显式挡住 —— 否则两条旋转叠加，胶囊会被转飞。
	if (HasAnimRootMotion())
	{
		return;
	}

	if (RotationAuthority == EACTRotationAuthority::AnimationCurve)
	{
		ApplyAnimationDrivenYaw(DeltaTime);
		return;
	}

	// ---------------------------------------------------------------- MovementComponent 模式
	if (!bAutoUpdateYaw)
	{
		return;
	}

	// 0 = 冻结；负值在引擎语义里是"瞬间转到位"，这里不支持，当成冻结。
	const float SpeedDegPerSec = RotationRate.Yaw;
	if (SpeedDegPerSec <= 0.0f)
	{
		return;
	}

	const float CurrentYaw = UpdatedComponent->GetComponentRotation().Yaw;
	const float NewYaw = FMath::FixedTurn(CurrentYaw, TargetYawAngle, SpeedDegPerSec * DeltaTime);

	if (FMath::IsNearlyEqual(NewYaw, CurrentYaw, 0.01f))
	{
		return;
	}

	MoveUpdatedComponent(FVector::ZeroVector, FQuat(FRotator(0.0f, NewYaw, 0.0f)), false);
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
			TargetYawAngle = FRotator::NormalizeAxis(OwningController->GetControlRotation().Yaw);
		}
		return;
	}

	// 用 GetLastMovementInputVector 而不是 Acceleration：前者在 Super::TickComponent 之前
	// 是"上一帧被消费的输入"，后者要到 PerformMovement 里才更新。只需要方向，
	// 晚一帧无所谓（转 90° 要 0.17 秒），但换来的是一个纯方向量、不受加速度模长影响。
	const FVector InputVector = CharacterOwner->GetLastMovementInputVector();

	// 0.01 = 模长 0.1，摇杆死区。和旧实现保持一致。
	if (InputVector.SizeSquared2D() > 0.01f)
	{
		TargetYawAngle = FRotator::NormalizeAxis(InputVector.ToOrientationRotator().Yaw);
	}
	else if (bHasRequestedVelocity && RequestedVelocity.SizeSquared() > UE_KINDA_SMALL_NUMBER)
	{
		// AI 寻路请求（RequestDirectMove）。保留引擎原本的这条分支，AI 不会退化。
		TargetYawAngle = FRotator::NormalizeAxis(RequestedVelocity.ToOrientationRotator().Yaw);
	}
	else if (Velocity.SizeSquared2D() > FMath::Square(MinSpeedToOrient))
	{
		// 松手但还在滑：朝速度方向。
		TargetYawAngle = FRotator::NormalizeAxis(Velocity.ToOrientationRotator().Yaw);
	}
	// 三个都为空：**保持上一次的目标角**，不要清零。
	//
	// 这一行"什么都不做"就是点按能转到位的全部原因：按一下 A 目标已是 90°，
	// 松手后输入没了、速度 14ms 就归零，但目标还留着，PhysicsRotation 会继续朝它转。
	// 之前有一版改成"每帧现算目标"，结果点按只能转 18° 就停 —— 别再改成那样。
}

void UACTCharacterMovementComponent::RefreshRotationRate()
{
	const bool bLookDirection = (RotationMode == EACTRotationMode::LookingDirection);

	// 只转 Yaw。角色恒竖直，不用管 Pitch/Roll。
	RotationRate = FRotator(0.0f, bLookDirection ? LookingDirectionInterpSpeed : VelocityDirectionInterpSpeed, 0.0f);
}

void UACTCharacterMovementComponent::SetRotationAuthority(EACTRotationAuthority NewAuthority)
{
	if (RotationAuthority == NewAuthority)
	{
		return;
	}

	if (NewAuthority == EACTRotationAuthority::AnimationCurve)
	{
		BeginAnimationDrivenYaw();
	}
	else
	{
		EndAnimationDrivenYaw();
	}
}

void UACTCharacterMovementComponent::BeginAnimationDrivenYaw()
{
	if (!CharacterOwner)
	{
		return;
	}

	RotationAuthority = EACTRotationAuthority::AnimationCurve;

	// 基准角取"当前朝向"，所以曲线的首帧偏移必须是 0，否则会跳一下。
	AnimDrivenEntryYaw = CharacterOwner->GetActorRotation().Yaw;
	AnimDrivenYaw = AnimDrivenEntryYaw;
	AnimYawSilenceTimer = 0.0f;

	// 目标角也停在这里。曲线模式结束后如果没有任何输入/速度，靠它保证角色不会
	// 突然转回"进入曲线模式之前"那个旧目标。
	TargetYawAngle = FRotator::NormalizeAxis(AnimDrivenEntryYaw);

	CachedAnimInstance = ResolveAnimInstance();
	if (CachedAnimInstance)
	{
		// 清掉上一轮可能残留的样本，并把序号对齐到当前值 —— 这样第一个新样本一定能被识别。
		CachedAnimInstance->ResetYawCurve();
		LastConsumedYawSeq = CachedAnimInstance->GetYawCurveSeq();
	}
	else
	{
		UE_LOG(LogACTMovement, Warning,
			TEXT("[%s] 进入曲线模式，但找不到 UACTAnimInstance（Owner 没有 Mesh，或 ABP 没继承它）。"
			     "将在首次 PhysicsRotation 时退回 MovementComponent。"),
			*GetNameSafe(CharacterOwner));
	}
}

void UACTCharacterMovementComponent::EndAnimationDrivenYaw()
{
	if (RotationAuthority != EACTRotationAuthority::AnimationCurve)
	{
		return;
	}

	RotationAuthority = EACTRotationAuthority::MovementComponent;
	AnimYawSilenceTimer = 0.0f;

	// 目标角对齐到"曲线转完后的实际朝向"：下一帧 UpdateDesiredYaw 若有输入/速度会立刻覆盖它，
	// 若没有（松手静止）就停在这里 —— 不会转回进入曲线模式之前那个旧目标。
	TargetYawAngle = CharacterOwner
		? FRotator::NormalizeAxis(CharacterOwner->GetActorRotation().Yaw)
		: TargetYawAngle;

	RefreshRotationRate();
}

void UACTCharacterMovementComponent::ApplyAnimationDrivenYaw(float DeltaTime)
{
	UACTAnimInstance* AnimInstance = ResolveAnimInstance();
	if (!AnimInstance)
	{
		// 没有动画实例（专用服务器、纯物理 Pawn）：曲线模式无从谈起。
		EndAnimationDrivenYaw();
		return;
	}

	// 外部改了朝向（传送、蓝图 SetActorRotation）：把基准一起搬过去，
	// 否则下一帧曲线会按老基准把角色拉回原位，看起来像"传送被撤销"。
	{
		const float ActorYaw = CharacterOwner->GetActorRotation().Yaw;
		const float Drift = FRotator::NormalizeAxis(AnimDrivenYaw - ActorYaw);
		if (FMath::Abs(Drift) > ExternalYawDriftThreshold)
		{
			AnimDrivenEntryYaw = FRotator::NormalizeAxis(AnimDrivenEntryYaw + Drift);
			AnimDrivenYaw = ActorYaw;
		}
	}

	float Offset = 0.0f;
	uint32 Seq = 0;
	const bool bActive = AnimInstance->ReadYawCurve(Offset, Seq);

	if (bActive && Seq != LastConsumedYawSeq)
	{
		LastConsumedYawSeq = Seq;
		AnimYawSilenceTimer = 0.0f;
		AnimDrivenYaw = FRotator::NormalizeAxis(AnimDrivenEntryYaw + Offset);
	}
	else
	{
		AnimYawSilenceTimer += DeltaTime;
		if (!bActive || AnimYawSilenceTimer > AnimationYawTimeout)
		{
			// 兜底：ABP 忘了 Clear、或状态机提前退出导致没人推数据。
			// 不这么做的话角色会永远卡在曲线最后一帧的朝向。
			UE_LOG(LogACTMovement, Warning,
				TEXT("[%s] 曲线模式结束（%s），退回 MovementComponent。检查 ABP 里 "
				     "PushYawCurveOffset / ClearYawCurveOffset 是否成对。"),
				*GetNameSafe(CharacterOwner),
				bActive ? TEXT("超时") : TEXT("已清除"));
			EndAnimationDrivenYaw();
			return;
		}
	}

	// 直接落位，不插值：曲线的每一帧就是这帧该处的朝向，再套一层 FixedTurn 等于把节奏
	// 又交回给 RotationRate，那曲线就白烘了。
	MoveUpdatedComponent(FVector::ZeroVector, FQuat(FRotator(0.0f, AnimDrivenYaw, 0.0f)), false);
}

void UACTCharacterMovementComponent::SnapYawTo(float NewYaw)
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	const float Yaw = FRotator::NormalizeAxis(NewYaw);
	MoveUpdatedComponent(FVector::ZeroVector, FQuat(FRotator(0.0f, Yaw, 0.0f)), false);

	// 目标角一起搬。否则下一帧 PhysicsRotation 看到"当前 90°、目标还是 0°"，
	// 会立刻把角色转回去 —— 折返瞬转就变成了"抖一下"。
	TargetYawAngle = Yaw;

	if (RotationAuthority == EACTRotationAuthority::AnimationCurve)
	{
		// 曲线模式的基准跟着搬，否则下一帧曲线会把角色拉回老方向。
		AnimDrivenEntryYaw = Yaw;
		AnimDrivenYaw = Yaw;
	}
}

void UACTCharacterMovementComponent::SetAutoYawUpdate(bool bEnable)
{
	// 只改开关，不用刷 RotationRate —— PhysicsRotation 每帧自己会看这个 bool。
	bAutoUpdateYaw = bEnable;
}

void UACTCharacterMovementComponent::SetRotationMode(EACTRotationMode NewMode)
{
	RotationMode = NewMode;
	RefreshRotationRate();
}

UACTAnimInstance* UACTCharacterMovementComponent::ResolveAnimInstance()
{
	if (IsValid(CachedAnimInstance))
	{
		return CachedAnimInstance;
	}

	CachedAnimInstance = nullptr;
	if (const USkeletalMeshComponent* Mesh = CharacterOwner ? CharacterOwner->GetMesh() : nullptr)
	{
		CachedAnimInstance = Cast<UACTAnimInstance>(Mesh->GetAnimInstance());
	}

	return CachedAnimInstance;
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

	// ---------------------------------------------------------------- 朝向修正角
	// Orientation Warping 的输入 = 「应该朝向」与「实际朝向」的差。
	// 这里直接用 PhysicsRotation 正在追的那个目标（TargetYawAngle / AnimDrivenYaw），
	// 天然同源。**不要在这里另算一套**（比如改用 Acceleration）—— 那会让动画去补一个
	// 物理已经在补的差，表现为动画比胶囊转得更远。
	const float ActorYaw = CharacterOwner->GetActorRotation().Yaw;
	const float DesiredYaw = (RotationAuthority == EACTRotationAuthority::AnimationCurve)
		? AnimDrivenYaw
		: TargetYawAngle;

	LocomotionState.YawOffset = FRotator::NormalizeAxis(DesiredYaw - ActorYaw);

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
