// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Character/Components/ActionLogicComponent.h"
#include "Character/Components/CharacterLogicDriverComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Character/IACTLogicCharacter.h"
#include "ACTGameCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AACTGameCharacter : public ACharacter, public IACTLogicCharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	UInputAction* MouseLookAction;

public:

	/** Constructor */
	AACTGameCharacter();	

	virtual void TickActor( float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction ) override;
protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	// class UCharacterLogicDriverComponent* CharacterLogicDriverComponent;
	class UInputToCommandComponent* InputToCommandComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UActionLogicComponent* ActionLogicComponent;

public:
	// FORCEINLINE class UCharacterLogicDriverComponent* GetCharacterLogicDriverComponent() const { return CharacterLogicDriverComponent; }

public:
	virtual void TickLogic(int64 CurrentFrame) override {};
    FORCEINLINE virtual uint32 GetObjectId() const override { return GetUniqueID(); }

    FORCEINLINE virtual class UInputToCommandComponent* GetInputToCommandComponent() const  override  { return InputToCommandComponent; }
    FORCEINLINE virtual class UActionLogicComponent* GetActionLogicComponent() const override  { return ActionLogicComponent; }
	virtual FVector ThisTickMove(int64 CurrentFrame) override;
	virtual void PrepareMovementForLogicFrame(int64 FrameNumber) override;
	virtual FLogicMovementIntent GetMovementIntent() const override;
	virtual class UCharacterMovementComponent* GetCharacterMovementComponent() const override { return GetCharacterMovement(); }
private:
	FVector NatureMove();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player")
	bool IsPlayer = true;
};

