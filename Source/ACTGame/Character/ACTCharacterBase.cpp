// Copyright Epic Games, Inc. All Rights Reserved.

#include "ACTCharacterBase.h"
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

#include "AlsCameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Utility/AlsVector.h"

DEFINE_LOG_CATEGORY(LogACTCharacterBase);

AACTCharacterBase::AACTCharacterBase()
{
    Camera = CreateDefaultSubobject<UAlsCameraComponent>(FName{ TEXTVIEW("Camera") });
    Camera->SetupAttachment(GetMesh());
    Camera->SetRelativeRotation_Direct({ 0.0f, 90.0f, 0.0f });
    Camera->SetRightShoulder(false);
    SetDesiredRotationMode(AlsRotationModeTags::VelocityDirection);
}

void AACTCharacterBase::NotifyControllerChanged()
{
    const auto* PreviousPlayer{ Cast<APlayerController>(PreviousController) };
    if (IsValid(PreviousPlayer))
    {
        auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PreviousPlayer->GetLocalPlayer()) };
        if (IsValid(InputSubsystem))
        {
            InputSubsystem->RemoveMappingContext(InputMappingContext);
        }
    }

    auto* NewPlayer{ Cast<APlayerController>(GetController()) };
    if (IsValid(NewPlayer))
    {
        NewPlayer->InputYawScale_DEPRECATED   = 1.0f;
        NewPlayer->InputPitchScale_DEPRECATED = 1.0f;
        NewPlayer->InputRollScale_DEPRECATED  = 1.0f;

        auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer()) };
        if (IsValid(InputSubsystem))
        {
            FModifyContextOptions Options;
            Options.bNotifyUserSettings = true;

            InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);
        }
    }

    Super::NotifyControllerChanged();
}

void AACTCharacterBase::CalcCamera(const float DeltaTime, FMinimalViewInfo& ViewInfo)
{
    if (Camera->IsActive())
    {
        Camera->GetViewInfo(ViewInfo);
        return;
    }

    Super::CalcCamera(DeltaTime, ViewInfo);
}

void AACTCharacterBase::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);

    auto* EnhancedInput{ Cast<UEnhancedInputComponent>(Input) };
    if (IsValid(EnhancedInput))
    {
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
        EnhancedInput->BindAction(RollAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRoll);
    }
}

void AACTCharacterBase::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
    const FVector2f Value{ ActionValue.Get<FVector2D>() };

    AddControllerPitchInput(Value.Y * LookUpMouseSensitivity);
    AddControllerYawInput(Value.X * LookRightMouseSensitivity);
}

void AACTCharacterBase::Input_OnLook(const FInputActionValue& ActionValue)
{
    const FVector2f Value{ ActionValue.Get<FVector2D>() };

    AddControllerPitchInput(Value.Y * LookUpRate);
    AddControllerYawInput(Value.X * LookRightRate);
}

void AACTCharacterBase::Input_OnMove(const FInputActionValue& ActionValue)
{
    const auto Value{ UAlsVector::ClampMagnitude012D(ActionValue.Get<FVector2D>()) };

    auto ViewRotation{ GetViewState().Rotation };

    if (IsValid(GetController()))
    {
        // Use exact camera rotation instead of target rotation whenever possible.

        FVector ViewLocation;
        GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
    }

    const auto ForwardDirection{ UAlsVector::AngleToDirectionXY(UE_REAL_TO_FLOAT(ViewRotation.Yaw)) };
    const auto RightDirection{ UAlsVector::PerpendicularCounterClockwiseXY(ForwardDirection) };

    AddMovementInput(ForwardDirection * Value.Y + RightDirection * Value.X);
}

void AACTCharacterBase::Input_OnSprint(const FInputActionValue& ActionValue)
{
    SetDesiredGait(ActionValue.Get<bool>() ? AlsGaitTags::Sprinting : AlsGaitTags::Running);
}

void AACTCharacterBase::Input_OnWalk()
{
    if (GetDesiredGait() == AlsGaitTags::Walking)
    {
        SetDesiredGait(AlsGaitTags::Running);
    }
    else if (GetDesiredGait() == AlsGaitTags::Running)
    {
        SetDesiredGait(AlsGaitTags::Walking);
    }
}

void AACTCharacterBase::Input_OnJump(const FInputActionValue& ActionValue)
{
    if (ActionValue.Get<bool>())
    {
        if (StopRagdolling())
        {
            return;
        }

        if (StartMantlingGrounded())
        {
            return;
        }

        if (GetStance() == AlsStanceTags::Crouching)
        {
            SetDesiredStance(AlsStanceTags::Standing);
            return;
        }

        Jump();
    }
    else
    {
        StopJumping();
    }
}

void AACTCharacterBase::Input_OnRoll()
{
    static constexpr auto PlayRate{ 1.3f };

    StartRolling(PlayRate);
}

