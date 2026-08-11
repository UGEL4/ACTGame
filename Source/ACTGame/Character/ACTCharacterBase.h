// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Character/ALSBaseCharacter.h"
#include "Character/Components/ActionLogicComponent.h"
#include "Character/Components/CharacterLogicDriverComponent.h"
//#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Character/IACTLogicCharacter.h"
#include "ACTCharacterBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogACTCharacterBase, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(Blueprintable, BlueprintType)
class AACTCharacterBase : public AALSBaseCharacter
{
	GENERATED_BODY()

public:
    AACTCharacterBase(const FObjectInitializer& ObjectInitializer);

    virtual void RagdollStart() override;
};

