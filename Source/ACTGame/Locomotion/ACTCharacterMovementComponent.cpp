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

	/** 输入方向角速度的平滑系数（1/s）。故意偏低：要滤掉摇杆噪声，只保留"跳变"。 */
	constexpr float TurnRateSmoothingRate = 12.0f;

	/** 转向速度折扣的平滑系数（1/s）。太快会让速度曲线出现台阶。 */
	constexpr float TurnSpeedSmoothingRate = 8.0f;

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

	// 初始化速度参数，避免第一帧 SmoothedMaxSpeed 从 0 爬上来（角色会先"起步慢半拍"）。
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
	// 先把动画线程写的原子槽搬进 GT 状态，后面所有逻辑才看得到本帧的驱动值。
	// 必须放在最前面：UpdateDesiredYaw / UpdateTurnState 都要读 bAnimYawDriven。
	ConsumeAnimationDrivenYaw(DeltaTime);

	// 先定方向、再定转向分级、再定速度参数，最后交给引擎做位移。
	// 顺序不能换：UpdateGaitParameters 要读 UpdateTurnState 算出的 TurnSpeedScale。
	UpdateDesiredYaw(DeltaTime);
	UpdateTurnState(DeltaTime);
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

	// 蒙太奇 root motion 自己带旋转，此时插一脚会导致朝向被覆盖或抖一下。
	// 但缓存必须跟着走：否则 RM 转完之后 CurrentYawAngle 还是旧值，下一帧 FixedTurn
	// 会从旧角度出发把角色硬拽回去（表现为翻滚/转身结束时"回弹一下"）。
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

	// pivot 确认窗口（TurnStableDwell）内暂停旋转。
	// 这几帧如果照常转，会白白吃掉 30~90° 转角：蒙太奇一开，clip 的 root motion 又从 0 转 180°，
	// 结果是"转过头 → 出蒙太奇后被 PhysicsRotation 拉回来"，肉眼可见地甩一下。
	// 只有请求角超过 PivotAngleThreshold 时才冻结（绕圈永远达不到），所以不影响正常转向。
	if (bPivotRotationFrozen)
	{
		return;
	}

	// 动画驱动朝向（状态机 pivot）。曲线给的是绝对角度，直接落位、不做插值 ——
	// 曲线本身已经含了"慢-快-慢"的节奏，再插值一次会把它抹平，看起来像匀速硬转。
	// 放在冻结判断**之后**：冻结只服务于"等确认"和"等 RM 接管"，与曲线驱动互斥。
	if (bAnimYawDriven)
	{
		CurrentYawAngle = FRotator::NormalizeAxis(AnimationDrivenYaw);
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

	float InterpSpeed = GetRotationInterpSpeed();

	// 大角度提速：基础 540deg/s 转 180° 要 0.33 秒，看起来像"舍不得转身"；
	// 1080 只要 0.17 秒。用插值而不是硬切，避免转到一半突然加速。
	// 注意覆盖值优先 —— 攻击锁朝向时 SetRotationInterpSpeed(0) 不能被这里顶掉。
	if (RotationInterpSpeedOverride < 0.0f)
	{
		const float AbsOffset = FMath::Abs(FRotator::NormalizeAxis(TargetYawAngle - CurrentYawAngle));
		const float SpeedBlend = FMath::Clamp(
			(AbsOffset - SharpTurnAngleThreshold) / FMath::Max(1.0f, 180.0f - SharpTurnAngleThreshold), 0.0f, 1.0f);
		InterpSpeed = FMath::Lerp(InterpSpeed, SharpTurnInterpSpeed, SpeedBlend);
	}

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

void UACTCharacterMovementComponent::UpdateDesiredYaw(float DeltaTime)
{
	bInputDrivenThisFrame = false;

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
	bInputDrivenThisFrame = InputVector.SizeSquared2D() > 0.01f;

	if (bInputDrivenThisFrame)
	{
		InputYaw = FRotator::NormalizeAxis(InputVector.ToOrientationRotator().Yaw);
		TargetYawAngle = InputYaw;
	}
	else if (Velocity.SizeSquared2D() > FMath::Square(MinSpeedToOrient))
	{
		// 松手滑行阶段没输入意图，保持"往哪滑就往哪"，不要突然回头。
		TargetYawAngle = FRotator::NormalizeAxis(Velocity.ToOrientationRotator().Yaw);
	}
	// 两者都没有：保持上一次的目标角（静止时不要抖）。

	// ---------------------------------------------------------------- 输入稳定性
	// 唯一职责是排除「绕圈/搓摇杆」：绕圈时输入方向每帧都在变，计时永远起不来。
	// 它**不承担**"等角度"的职责 —— 角度参照的是速度方向，反向过程中天然稳定（见 UpdateTurnState）。
	if (!bInputDrivenThisFrame)
	{
		// 松手后不能把速度方向记成输入历史，否则重新推摇杆会凭空多出一个大跳变。
		TurnStableTimer = 0.0f;
		bHasPrevInputYaw = false;
		return;
	}

	if (DeltaTime > KINDA_SMALL_NUMBER)
	{
		const float FrameDelta = bHasPrevInputYaw
			? FMath::Abs(FRotator::NormalizeAxis(InputYaw - PrevInputYaw))
			: 0.0f;

		TurnStableTimer = FrameDelta > TurnInputSettleAngle ? 0.0f : TurnStableTimer + DeltaTime;

		// 诊断用：输入方向角速度。绕圈会稳定在几百到一千多，反打是单帧尖峰后归零。
		if (bHasPrevInputYaw)
		{
			const float RawRate = FrameDelta / DeltaTime;
			SmoothedTurnRate = FMath::FInterpTo(SmoothedTurnRate, RawRate, DeltaTime, TurnRateSmoothingRate);
		}
	}

	PrevInputYaw = InputYaw;
	bHasPrevInputYaw = true;
}

void UACTCharacterMovementComponent::UpdateTurnState(float DeltaTime)
{
	if (!CharacterOwner)
	{
		return;
	}

	PivotCooldownTimer = FMath::Max(0.0f, PivotCooldownTimer - DeltaTime);
	PivotFreezeTimer = FMath::Max(0.0f, PivotFreezeTimer - DeltaTime);

	// ---------------------------------------------------------------- 两个角度，参照系不同
	// ResidualAngle = 目标朝向 vs 实际朝向。身体 0.17 秒就转完，是一条**斜坡**（180→90→0）。
	//                 用途：速度包络、动画层 YawOffset。
	// TurnAngle     = 输入意图 vs **速度方向**。速度反向要 0.65 秒，而且中途是**阶跃**
	//                 （前 0.3 秒一直是 180°，速度过零后才瞬间变 0）。用途：pivot 判定。
	//
	// 为什么判定必须用速度做参照：用朝向做参照时，任何采样延迟都会读到已经衰减的中间值
	// （这正是 pivot 一直触发不了的原因）；用速度做参照，整个反向窗口内读到的都是稳定的 180。
	// 这也是 Dot(Velocity, Acceleration) < 0 的完整形式 —— 点乘只给出"夹角 > 90°"这个符号，
	// 而 140° 的门槛比它严格，所以不需要再单独判点乘。
	const float ResidualAngle = FRotator::NormalizeAxis(TargetYawAngle - CharacterOwner->GetActorRotation().Yaw);

	float TurnAngle = ResidualAngle;
	if (bInputDrivenThisFrame && Velocity.SizeSquared2D() > FMath::Square(MinSpeedToOrient))
	{
		TurnAngle = FRotator::NormalizeAxis(InputYaw - Velocity.ToOrientationRotator().Yaw);
	}

	const float AbsTurnAngle = FMath::Abs(TurnAngle);
	const float AbsResidual = FMath::Abs(ResidualAngle);

	const float CurrentSpeed2D = Velocity.Size2D();

	// ---------------------------------------------------------------- pivot 判定
	//   1. 只在"朝速度方向"模式下有意义 ——
	//      LookingDirection 的目标角来自相机，绕着角色转一圈相机就会造出 180° 的假反打；
	//      bAutoUpdateYaw == false 时目标角归外部（攻击锁定）管，也不该播 pivot。
	//   2. 必须有输入意图，且输入与速度的夹角够大。这个角在反向过程中天然稳定，不需要锁存。
	//   3. 速度必须在窗口内：clip 是按固定速度 K 的，进出速度不匹配会让 RM 硬拉速度。
	//   4. 冷却已过，保证不会连播。
	//   5. 在地上。
	//   6. 输入方向已稳定（TurnStableTimer），排除绕圈/搓摇杆。
	const bool bPivotMode = bAutoUpdateYaw && RotationMode == EACTRotationMode::VelocityDirection;

	const bool bPivotPending =
		bPivotMode
		&& !bAnimYawDriven
		&& bInputDrivenThisFrame
		&& AbsTurnAngle > PivotAngleThreshold
		&& CurrentSpeed2D > PivotMinSpeed
		&& CurrentSpeed2D < PivotMaxSpeed
		&& PivotCooldownTimer <= 0.0f
		&& IsMovingOnGround();

	if (bPivotPending && TurnStableTimer >= TurnStableDwell)
	{
		PivotCooldownTimer = PivotCooldown;
		TurnStableTimer = 0.0f;

		// 等蒙太奇的 root motion 真正接管之前不要再扭胶囊，否则 clip 会转过头。
		PivotFreezeTimer = 0.08f;

		// 广播"相对当前朝向"的带符号角度：它决定播左版还是右版。
		// 此刻旋转是冻结的，所以它和 AbsTurnAngle 基本相等。
		const float BroadcastAngle = FRotator::NormalizeAxis(TargetYawAngle - CharacterOwner->GetActorRotation().Yaw);

		UE_LOG(LogACTMovement, Log, TEXT("[%s] Pivot: angle=%.1f (vs velocity) residual=%.1f speed=%.0f"),
			*GetNameSafe(CharacterOwner), TurnAngle, ResidualAngle, CurrentSpeed2D);

		OnPivotRequested.Broadcast(BroadcastAngle);
	}

	// 冻结条件：正在等确认，或刚触发完还没等到 RM 接管。
	bPivotRotationFrozen = bPivotPending || PivotFreezeTimer > 0.0f;

	// ---------------------------------------------------------------- 速度折扣
	// 用 ResidualAngle（实时剩余角）：折扣要跟着身体真实转过多少走，而不是跟着"想转多少"。
	//
	// 两种情况下折扣必须让路（Velocity 由 root motion 接管，折扣要么无效、要么有害）：
	//   1. 蒙太奇正在播 —— 引擎把 Velocity 整个换成 RM 速度（ConstrainAnimRootMotionVelocity）
	//   2. **马上就要播** —— 候选成立、只是在等稳定确认。
	//      漏了这一条，确认期间速度被压到 330，蒙太奇一开 RM 又拉回 500，进入瞬间会明显加速。
	//
	// 反过来：绕圈时稳定计时永远起不来 → bPivotPending 恒 false → 折扣全量生效，正是想要的。
	//
	// 注意 bAnimYawDriven **不在这里**：状态机 pivot 只接管旋转，位移仍然由 velocity 提供，
	// 所以更需要减速 —— 不然身体 180° 转过去的同时还在以 500 前进，脚必然大面积打滑。
	// 折扣的角度用的是 ResidualAngle，而曲线驱动时胶囊在转，残差自然从 180 收窄，
	// 折扣也就跟着动画节奏自动放开，不需要额外处理。
	const bool bPivotOwnsMotion = HasAnimRootMotion() || bPivotPending;

	const float BrakeAlpha = FMath::Clamp(
		(AbsResidual - TurnBrakeStartAngle) / FMath::Max(1.0f, 180.0f - TurnBrakeStartAngle), 0.0f, 1.0f);
	const float TargetScale = bPivotOwnsMotion ? 1.0f : FMath::Lerp(1.0f, TurnBrakeMinScale, BrakeAlpha);

	TurnSpeedScale = FMath::FInterpTo(TurnSpeedScale, TargetScale, DeltaTime, TurnSpeedSmoothingRate);
}

float UACTCharacterMovementComponent::GetTurnAngle() const
{
	if (!CharacterOwner)
	{
		return 0.0f;
	}

	return FRotator::NormalizeAxis(TargetYawAngle - CharacterOwner->GetActorRotation().Yaw);
}

bool UACTCharacterMovementComponent::IsSharpTurning() const
{
	return FMath::Abs(GetTurnAngle()) > SharpTurnAngleThreshold;
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

// ================================================================ 动画驱动朝向（跨线程）

void UACTCharacterMovementComponent::ConsumeAnimationDrivenYaw(float DeltaTime)
{
	if (!CharacterOwner)
	{
		return;
	}

	// 数据入口在 AnimInstance 上，因为我们这边拿不到线程安全的访问路径：
	// BlueprintThreadSafeUpdateAnimation 里只有 self，要从 self 走到移动组件必须经过
	// GetCharacterMovement，而它没有 BlueprintThreadSafe 标记。所以由动画写自己的原子槽，
	// 这里在 GT 上拉。全程没有跨对象的 UFUNCTION 调用。
	const UACTAnimInstance* AnimInstance = nullptr;
	if (const USkeletalMeshComponent* Mesh = CharacterOwner->GetMesh())
	{
		AnimInstance = Cast<UACTAnimInstance>(Mesh->GetAnimInstance());
	}

	const bool bWantsDriven = AnimInstance && AnimInstance->IsAnimYawDriveActive();
	const uint32 Seq = AnimInstance ? AnimInstance->GetAnimYawDriveSeq() : 0;

	if (!bWantsDriven)
	{
		EndAnimationDrivenYaw();
		LastAnimYawSeq = Seq;
		AnimYawIdleTimer = 0.0f;
		return;
	}

	// 看门狗：状态函数每帧都会写一次，序列号不变说明动画那边已经不再喂值了
	// （状态被中断、ABP 忘了调 StopAnimationDrivenYaw、或者状态机已经切走）。这时必须自己收尾，
	// 否则 PhysicsRotation 会永久停在曲线分支，角色再也不会朝输入方向转。
	AnimYawIdleTimer = (Seq != LastAnimYawSeq) ? 0.0f : AnimYawIdleTimer + DeltaTime;
	LastAnimYawSeq = Seq;

	if (AnimYawIdleTimer > AnimYawTimeout)
	{
		UE_LOG(LogACTMovement, Warning,
			TEXT("[%s] 动画驱动朝向超时 %.2fs 没有收到新值，自动交还控制权。")
			TEXT("检查 pivot 状态的 On State Exit 是否调了 StopAnimationDrivenYaw。"),
			*GetNameSafe(CharacterOwner), AnimYawTimeout);

		EndAnimationDrivenYaw();
		return;
	}

	if (!bAnimYawDriven)
	{
		// 基准角只能在 GT 上取。动画送来的偏移量是相对这个角度的，
		// 所以曲线首帧必须是 0（modifier 勾 bRelativeToFirstFrame，或 ABP 自己减首帧值）。
		bAnimYawDriven = true;
		AnimDrivenEntryYaw = CharacterOwner->GetActorRotation().Yaw;
		CurrentYawAngle = AnimDrivenEntryYaw;

		UE_LOG(LogACTMovement, Verbose, TEXT("[%s] Anim yaw driven ON, entry=%.1f"),
			*GetNameSafe(CharacterOwner), AnimDrivenEntryYaw);
	}

	AnimationDrivenYaw = FRotator::NormalizeAxis(
		AnimDrivenEntryYaw + AnimInstance->GetAnimYawDriveOffset());
}

void UACTCharacterMovementComponent::EndAnimationDrivenYaw()
{
	if (!bAnimYawDriven)
	{
		return;
	}

	bAnimYawDriven = false;

	if (CharacterOwner)
	{
		// 胶囊已经被曲线转到位，缓存跟着走；目标角也拉平，避免退出瞬间被过期的目标角拽一下。
		CurrentYawAngle = CharacterOwner->GetActorRotation().Yaw;
		TargetYawAngle = CurrentYawAngle;
	}

	UE_LOG(LogACTMovement, Verbose, TEXT("[%s] Anim yaw driven OFF at %.1f"),
		*GetNameSafe(CharacterOwner), CurrentYawAngle);
}

void UACTCharacterMovementComponent::SetAutoYawUpdate(bool bEnable)
{
	bAutoUpdateYaw = bEnable;
}

void UACTCharacterMovementComponent::SetRotationInterpSpeed(float NewSpeed)
{
	//RotationInterpSpeedOverride = FMath::Max(0.0f, NewSpeed);
    // 负数 = 恢复 RotationMode 默认值。不能钳到 0 —— 0 的语义是「冻结旋转」，
    // 钳过去会让 Pivot / 攻击结束后无法恢复默认转向速度。
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

	// 转向折扣只乘在"最终上限"上，不回写 SmoothedMaxSpeed ——
	// 否则折扣会被步态插值平滑两次，转身结束时速度恢复会拖一条长尾。
	// 效果：MaxWalkSpeed 降低 → CalcVelocity 里的 IsExceedingMaxSpeed 触发正常刹车
	// （BrakingDecelerationWalking），所以是"减速"而不是"硬钳"。
	MaxWalkSpeed = SmoothedMaxSpeed * TurnSpeedScale;
	MaxWalkSpeedCrouched = SmoothedMaxSpeed * FMath::Clamp(CrouchSpeedScale, 0.0f, 1.0f) * TurnSpeedScale;

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
	// 这里由"实际角 - 目标角"直接得出，不需要美术 K 曲线。
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
