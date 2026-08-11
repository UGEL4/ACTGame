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
#include "AI/ALSAIController.h"

DEFINE_LOG_CATEGORY(LogACTCharacterBase);

AACTCharacterBase::AACTCharacterBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AIControllerClass = AALSAIController::StaticClass();
}

void AACTCharacterBase::RagdollStart()
{
    Super::RagdollStart();
}

