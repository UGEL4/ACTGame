#include "ACTCharacterBase.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Locomotion/ACTCharacterMovementComponent.h"
#include "Math/RotationMatrix.h"

DEFINE_LOG_CATEGORY(LogACTCharacterBase);

AACTCharacterBase::AACTCharacterBase(const FObjectInitializer& ObjectInitializer)
	// 关键：ACharacter 在基类构造函数里已经用 CharacterMovementComponentName 这个名字创建了
	// UCharacterMovementComponent。SetDefaultSubobjectClass 让同一个子对象位置换成我们的类，
	// 而不是再 CreateDefaultSubobject 一个（那样会有两个移动组件，且这个仍是基类实例）。
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UACTCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// 旋转完全交给移动组件的 PhysicsRotation()。这三项如果留着 true，
	// 角色会瞬间吸附到控制器朝向，移动组件的平滑转向（以及动画的 Orientation Warping）全部失效。
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12.0f;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;
	// 旋转也做 lag 会让瞄准发飘，ACT 里更需要跟手，所以只对位置做平滑。
	CameraBoom->bEnableCameraRotationLag = false;
	// 过肩偏移：纯正后方的相机在近距离战斗时会挡住敌人和攻击判定范围。
	CameraBoom->SocketOffset = FVector(0.0f, 40.0f, 60.0f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = 90.0f;

	// 蹲下开关。FNavAgentProperties::bCanCrouch 默认是 false，不打开的话
	// ACharacter::Crouch() 会静默失败（CanCrouch() 返回 false），表现为"按蹲没反应"。
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
		Movement->SetCrouchedHalfHeight(60.0f);
	}
}

void AACTCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ACTMovement = Cast<UACTCharacterMovementComponent>(GetCharacterMovement());
	if (!IsValid(ACTMovement))
	{
		UE_LOG(LogACTCharacterBase, Error,
			TEXT("[%s] CharacterMovement 不是 UACTCharacterMovementComponent。"
			     "检查构造函数里的 SetDefaultSubobjectClass，或者蓝图里是否覆盖了移动组件类。"),
			*GetNameSafe(this));
	}
	else
	{
		// 用事件而不是每帧轮询：pivot 是物理侧判定、动画侧执行，两边都不该知道对方的细节。
		ACTMovement->OnPivotRequested.AddDynamic(this, &AACTCharacterBase::HandlePivotRequested);
	}
}

void AACTCharacterBase::HandlePivotRequested(float TurnAngle)
{
	// 已经在播别的动作（翻滚、攻击）就不要插队 —— 那些蒙太奇自己带 root motion，
	// 两个 RM 叠加会把胶囊转飞。
	if (const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (AnimInstance->IsAnyMontagePlaying())
		{
			return;
		}
	}

	// 正 Yaw = 向左转（UE 的 yaw 逆时针为正）。所以 TurnAngle > 0 播左转版本。
	const bool bTurnLeft = TurnAngle > 0.0f;
	UAnimMontage* Montage = bTurnLeft ? PivotMontageLeft : PivotMontageRight;

	if (!IsValid(Montage))
	{
		// 没配资产不是错误：此时只靠旋转提速 + 速度折扣完成转身，观感差一些但能用。
		UE_LOG(LogACTCharacterBase, Verbose,
			TEXT("[%s] 未设置 %s，只用旋转完成 %.0f 度转身。"),
			*GetNameSafe(this), bTurnLeft ? TEXT("PivotMontageLeft") : TEXT("PivotMontageRight"), TurnAngle);
		return;
	}

	PlayAnimMontage(Montage, 1.0f);
}

// ==================================================================== 控制器 / 输入上下文

void AACTCharacterBase::NotifyControllerChanged()
{
	if (IsValid(InputMappingContext))
	{
		if (const APlayerController* PreviousPlayer = Cast<APlayerController>(PreviousController))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PreviousPlayer->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(InputMappingContext);
			}
		}

		if (const APlayerController* NewPlayer = Cast<APlayerController>(GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer()))
			{
				FModifyContextOptions Options;
				Options.bNotifyUserSettings = true;
				Subsystem->AddMappingContext(InputMappingContext, 0, Options);
			}
		}
	}

	Super::NotifyControllerChanged();
}

void AACTCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!IsValid(EnhancedInput))
	{
		UE_LOG(LogACTCharacterBase, Error,
			TEXT("[%s] InputComponent 不是 UEnhancedInputComponent。检查 DefaultInput.ini 的 "
			     "DefaultPlayerInputClass / DefaultInputComponentClass。"),
			*GetNameSafe(this));
		return;
	}

	// 绑定表沿用 ALS Example 角色的写法：
	//   Triggered + Canceled 成对绑定输入量，Canceled 负责在松手时把输入归零
	//   （只绑 Triggered 的话，松开摇杆后 Move 会停在最后一帧的方向上）。
	EnhancedInput->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLookMouse);
	EnhancedInput->BindAction(LookMouseAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnLookMouse);

	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLook);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnLook);

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnMove);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnMove);

	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnSprint);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnSprint);

	EnhancedInput->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnWalk);

	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnJump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnJump);

	EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnCrouch);
	EnhancedInput->BindAction(RollAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRoll);
}

// ==================================================================== 视角

void AACTCharacterBase::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
	const FVector2D Value = ActionValue.Get<FVector2D>();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}

	// 不用 AddControllerYawInput / AddControllerPitchInput：它们会乘上
	// APlayerController::InputYawScale_DEPRECATED / InputPitchScale_DEPRECATED（默认 -2.5 之类），
	// 还得在 NotifyControllerChanged 里手动改成 1.0。直接写 ControlRotation 更可控，
	// 也能顺手做俯仰夹取。
	FRotator NewRotation = PlayerController->GetControlRotation();
	NewRotation.Yaw += Value.X * LookRightMouseSensitivity;
	NewRotation.Pitch = FMath::ClampAngle(NewRotation.Pitch + Value.Y * LookUpMouseSensitivity, MinLookPitch, MaxLookPitch);
	NewRotation.Roll = 0.0f;

	PlayerController->SetControlRotation(NewRotation);
}

void AACTCharacterBase::Input_OnLook(const FInputActionValue& ActionValue)
{
	const FVector2D Value = ActionValue.Get<FVector2D>();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!IsValid(PlayerController))
	{
		return;
	}

	const float DeltaTime = GetWorld()->GetDeltaSeconds();

	FRotator NewRotation = PlayerController->GetControlRotation();
	NewRotation.Yaw += Value.X * LookRightRate * DeltaTime;
	NewRotation.Pitch = FMath::ClampAngle(NewRotation.Pitch + Value.Y * LookUpRate * DeltaTime, MinLookPitch, MaxLookPitch);
	NewRotation.Roll = 0.0f;

	PlayerController->SetControlRotation(NewRotation);
}

// ==================================================================== 移动

void AACTCharacterBase::Input_OnMove(const FInputActionValue& ActionValue)
{
	// 摇杆推到底允许长度 1，斜向推时长度会超过 1，钳一下避免斜着走更快。
	FVector2D Value = ActionValue.Get<FVector2D>().GetClampedToMaxSize(1.0f);

	if (!IsValid(Controller))
	{
		return;
	}

	// 只用 Yaw。如果带上相机的 Pitch，前进方向会带一个 -Z 分量，
	// CMC 会把水平速度按比例归一化掉一部分，表现为"低头看地时跑得更慢"。
	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, Value.Y);
	AddMovementInput(RightDirection, Value.X);
}

void AACTCharacterBase::Input_OnSprint(const FInputActionValue& ActionValue)
{
	SetGait(ActionValue.Get<bool>() ? EACTGait::Sprint : (bDesiredWalk ? EACTGait::Walk : EACTGait::Run));
}

void AACTCharacterBase::Input_OnWalk()
{
	bDesiredWalk = !bDesiredWalk;

	// 冲刺中按下走路键只改变偏好，等松开冲刺才生效。
	if (GetGait() != EACTGait::Sprint)
	{
		SetGait(bDesiredWalk ? EACTGait::Walk : EACTGait::Run);
	}
}

void AACTCharacterBase::Input_OnJump(const FInputActionValue& ActionValue)
{
	if (ActionValue.Get<bool>())
	{
		// 蹲着按跳 → 先站起来并吃掉这次输入，避免蹲姿起跳穿模。
		if (GetStance() == EACTStance::Crouch)
		{
			SetStance(EACTStance::Stand);
			return;
		}

		Jump();
		return;
	}

	StopJumping();
}

void AACTCharacterBase::Input_OnCrouch()
{
	SetStance(GetStance() == EACTStance::Crouch ? EACTStance::Stand : EACTStance::Crouch);
}

void AACTCharacterBase::Input_OnRoll()
{
	// 临时实现：Phase 6 接入 ActionLogicComponent 后，这里改为派发动作请求，
	// 由 ActionInfoAsset 决定播哪条蒙太奇、能不能打断、位移怎么给。
	if (!IsValid(RollMontage))
	{
		UE_LOG(LogACTCharacterBase, Warning, TEXT("[%s] 未设置 RollMontage，翻滚被忽略。"), *GetNameSafe(this));
		return;
	}

	if (GetStance() == EACTStance::Crouch)
	{
		SetStance(EACTStance::Stand);
	}

	PlayAnimMontage(RollMontage, RollPlayRate);
}

// ==================================================================== 状态转发

UACTCharacterMovementComponent* AACTCharacterBase::GetACTMovement() const
{
	if (IsValid(ACTMovement))
	{
		return ACTMovement;
	}

	// 兜底：PostInitializeComponents 之前调用也能拿到（例如构造函数外的纯数据访问）。
	return Cast<UACTCharacterMovementComponent>(GetCharacterMovement());
}

void AACTCharacterBase::SetGait(EACTGait NewGait)
{
	if (UACTCharacterMovementComponent* Movement = GetACTMovement())
	{
		Movement->SetGait(NewGait);
	}
}

EACTGait AACTCharacterBase::GetGait() const
{
	const UACTCharacterMovementComponent* Movement = GetACTMovement();
	return IsValid(Movement) ? Movement->GetGait() : EACTGait::Run;
}

void AACTCharacterBase::SetStance(EACTStance NewStance)
{
	if (UACTCharacterMovementComponent* Movement = GetACTMovement())
	{
		Movement->SetStance(NewStance);
	}
}

EACTStance AACTCharacterBase::GetStance() const
{
	const UACTCharacterMovementComponent* Movement = GetACTMovement();
	return IsValid(Movement) ? Movement->GetStance() : EACTStance::Stand;
}

void AACTCharacterBase::SetRotationMode(EACTRotationMode NewRotationMode)
{
	if (UACTCharacterMovementComponent* Movement = GetACTMovement())
	{
		Movement->SetRotationMode(NewRotationMode);
	}
}

EACTRotationMode AACTCharacterBase::GetRotationMode() const
{
	const UACTCharacterMovementComponent* Movement = GetACTMovement();
	return IsValid(Movement) ? Movement->GetRotationMode() : EACTRotationMode::VelocityDirection;
}

FActLocomotionState AACTCharacterBase::GetLocomotionState() const
{
	if (const UACTCharacterMovementComponent* Movement = GetACTMovement())
	{
		return Movement->GetLocomotionState();
	}

	return FActLocomotionState();
}
