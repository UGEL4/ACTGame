// Copyright Epic Games, Inc. All Rights Reserved.

#include "ACTGameCharacter.h"
#include "Engine/EngineTypes.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Framework/Game/Command/ActionCommand.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Character/Components/InputToCommandComponent.h"
#include "Character/Components/ActionLogicComponent.h"
#include "Character/Components/CharacterLogicDriverComponent.h"
#include "World/ACTWorldSubsystem.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AACTGameCharacter::AACTGameCharacter()
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

    CharacterLogicDriverComponent = CreateDefaultSubobject<UCharacterLogicDriverComponent>(TEXT("CharacterLogicDriverComponent"));
    InputToCommandComponent       = CreateDefaultSubobject<UInputToCommandComponent>(TEXT("InputToCommandComponent"));
    ActionLogicComponent          = CreateDefaultSubobject<UActionLogicComponent>(TEXT("ActionLogicComponent"));
}

void AACTGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AACTGameCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AACTGameCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AACTGameCharacter::Look);

		// ButtonX
        EnhancedInputComponent->BindAction(ButtonXAction, ETriggerEvent::Started, this, &AACTGameCharacter::OnButtonX);
		// ButtonY
        EnhancedInputComponent->BindAction(ButtonYAction, ETriggerEvent::Started, this, &AACTGameCharacter::OnButtonY);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AACTGameCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (ActionLogicComponent != nullptr && InputToCommandComponent != nullptr)
    {
        // InputToCommandComponent->AddTickPrerequisiteComponent(FrameComponent);
        ActionLogicComponent->AddTickPrerequisiteComponent(InputToCommandComponent);
    }
    if (auto Move = GetCharacterMovementComponent())
    {
        Move->Deactivate();
        if (ActionLogicComponent)
        {
            //Move->AddTickPrerequisiteComponent(ActionLogicComponent);
        }
    }

	//GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	//GetCharacterMovement()->Deactivate();

	/*if (auto LogicWorld = GetWorld()->GetSubsystem<UACTWorldSubsystem>())
	{
		LogicWorld->RegisterCharacter(this, IsPlayer);
	}*/
}

void AACTGameCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	/*if (auto LogicWorld = GetWorld()->GetSubsystem<UACTWorldSubsystem>())
	{
		LogicWorld->UnregisterCharacter(this, IsPlayer);
	}*/
}

void AACTGameCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);

	if (InputToCommandComponent != nullptr)
	{
		InputToCommandComponent->TryAddDirectInput(MovementVector);
	}
}

void AACTGameCharacter::TickActor( float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction )
{
	Super::TickActor( DeltaTime,   TickType,  ThisTickFunction);
}

void AACTGameCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AACTGameCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AACTGameCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AACTGameCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();

	if (InputToCommandComponent != nullptr)
	{
		InputToCommandComponent->TryAddInput(EKeyMap::ButtonSouth);
	}
}

void AACTGameCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

FVector AACTGameCharacter::ThisTickMove(int64 CurrentFrame)
{
	return NatureMove();
}

FVector AACTGameCharacter::NatureMove()
{
	return FVector::ZeroVector;
}

void AACTGameCharacter::PrepareMovementForLogicFrame(int64 FrameNumber)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		FVector RootMotionMove = ActionLogicComponent->GetRootMotionMove();
		// add movement 
		//AddMovementInput(ForwardDirection, Forward);
		//AddMovementInput(RightDirection, Right);
	}
}

FLogicMovementIntent AACTGameCharacter::GetMovementIntent() const
{
	return FLogicMovementIntent();
}

void AACTGameCharacter::OnButtonX()
{
	if (InputToCommandComponent)
	{
        InputToCommandComponent->TryAddInput(EKeyMap::ButtonWest);
	}
}

void AACTGameCharacter::OnButtonY()
{
    if (InputToCommandComponent)
    {
        InputToCommandComponent->TryAddInput(EKeyMap::ButtonNouth);
    }
}
