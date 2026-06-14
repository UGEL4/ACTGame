#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/Interface.h"
#include "IACTLogicCharacter.generated.h"

// 返回本帧期望的移动数据
struct FLogicMovementIntent
{
    FVector DeltaTranslation = FVector(0.5, 0, 0);   // 绝对位移
    FQuat DeltaRotation = FQuat::Identity;            // 目标朝向
    bool bHasMovement = false;
};

UINTERFACE(MinimalAPI, NotBlueprintable)
class UACTLogicCharacter : public UInterface
{
	GENERATED_BODY()
};

class IACTLogicCharacter
{
	GENERATED_BODY()
public:
	virtual void TickLogic(int64 CurrentFrame) = 0;
    virtual uint32 GetObjectId() const = 0;
    virtual class UInputToCommandComponent* GetInputToCommandComponent() const = 0;
    virtual class UActionLogicComponent* GetActionLogicComponent() const = 0;
    virtual FVector ThisTickMove(int64 CurrentFrame) = 0;
    virtual void PrepareMovementForLogicFrame(int64 FrameNumber) = 0;
    virtual FLogicMovementIntent GetMovementIntent() const = 0;
    virtual class UCharacterMovementComponent* GetCharacterMovementComponent() const = 0;
};

