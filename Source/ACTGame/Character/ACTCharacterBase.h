// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AlsCharacter.h"
#include "ACTCharacterBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UInputMappingContext;
class UAlsCameraComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogACTCharacterBase, Log, All);


/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(AutoExpandCategories = ("Settings|ACTCharacter Base"))
class ACTGAME_API AACTCharacterBase : public AAlsCharacter
{
	GENERATED_BODY()

public:
    AACTCharacterBase();

    virtual void NotifyControllerChanged() override;

    // Camera

protected:
    virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& ViewInfo) override;

    // Input

protected:
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

protected:
    virtual void Input_OnLookMouse(const FInputActionValue& ActionValue);

    virtual void Input_OnLook(const FInputActionValue& ActionValue);

    virtual void Input_OnMove(const FInputActionValue& ActionValue);

    virtual void Input_OnSprint(const FInputActionValue& ActionValue);

    virtual void Input_OnWalk();

    virtual void Input_OnJump(const FInputActionValue& ActionValue);

    virtual void Input_OnRoll();

protected:
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Als Character Example")
    TObjectPtr<UAlsCameraComponent> Camera;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
    TObjectPtr<UInputMappingContext> InputMappingContext;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
    TObjectPtr<UInputAction> LookMouseAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
    TObjectPtr<UInputAction> WalkAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
    TObjectPtr<UInputAction> RollAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example", Meta = (ClampMin = 0, ForceUnits = "x"))
    float LookUpMouseSensitivity{ 1.0f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example", Meta = (ClampMin = 0, ForceUnits = "x"))
    float LookRightMouseSensitivity{ 1.0f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
    float LookUpRate{ 90.0f };

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
    float LookRightRate{ 240.0f };
};

