// Copyright Epic Games, Inc. All Rights Reserved.

#include "ACTGamePlayerBase.h"
#include "Engine/EngineTypes.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

AACTGamePlayerBase::AACTGamePlayerBase()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AACTGamePlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AACTGamePlayerBase::Move);
        EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AACTGamePlayerBase::Look);

		// Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AACTGamePlayerBase::Look);
	}
}

void AACTGamePlayerBase::BeginPlay()
{
    Super::BeginPlay();
}

void AACTGamePlayerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AACTGamePlayerBase::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AACTGamePlayerBase::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor( DeltaTime,   TickType,  ThisTickFunction);
}

void AACTGamePlayerBase::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AACTGamePlayerBase::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
        FVector MakeTemp(Forward, Right, 0);
        //FRotator Rot   = UKismetMathLibrary::MakeRotFromX(GetControlRotation().RotateVector(Temp));
        const FRotator ControlRotation = GetControlRotation();
        FRotator RotFromX              = UKismetMathLibrary::MakeRotFromX(UKismetMathLibrary::GreaterGreater_VectorRotator(MakeTemp, ControlRotation));
        InputDirection                 = UKismetMathLibrary::NormalizedDeltaRotator(RotFromX, GetActorRotation()).Yaw;

        //UKismetSystemLibrary::DrawDebugCoordinateSystem(this, GetActorLocation(), RotFromX, 200.f, 0.0f, 10.f);

        float Yaw = UKismetMathLibrary::ComposeRotators(GetControlRotation(), FRotator(0, InputDirection > 0 ? -1 : 1, 0)).Yaw;

		FVector WorldInput = ControlRotation.RotateVector(FVector(Forward, Right, 0));
        WorldInput.Z       = 0;
        WorldInput.Normalize();

        const FVector ActorRight   = GetActorRightVector();
        const FVector ActorForward = GetActorForwardVector();

        float RightDot   = FVector::DotProduct(WorldInput, ActorRight);
        float ForwardDot = FVector::DotProduct(WorldInput, ActorForward);
        InputDirection   = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
        if (FMath::Abs(FMath::Abs(InputDirection) - 180.f) < 1.f)
        {
            // 用原始输入向量的 Right 分量决定方向，避免依赖浮点符号
            InputDirection = (Right >= 0.f) ? 180.f : -180.f;
        }

		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        //const FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(FRotator(0, Yaw, 0));

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
        //const FVector RightDirection = UKismetMathLibrary::GetRightVector(FRotator(0, Yaw, 0));

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AACTGamePlayerBase::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AACTGamePlayerBase::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AACTGamePlayerBase::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
