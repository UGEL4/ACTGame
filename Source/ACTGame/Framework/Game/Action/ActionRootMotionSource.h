#pragma once

#include "Asset/ActionInfo/RootMotionAsset.h"
#include "CoreMinimal.h"
#include "GameFramework/RootMotionSource.h"
#include "ActionRootMotionSource.generated.h"

USTRUCT()
struct FActionRootMotionSource : public FRootMotionSource
{
    GENERATED_BODY()

    UPROPERTY()
    const URootMotionAsset* SequenceData;

    float CurrentTime  = 0.0f;
    int32 CurrentFrameIndex = 0;

    virtual void PrepareRootMotion(float SimulationTime,
                                   float MovementTickTime,
                                   const ACharacter& Character,
                                   const UCharacterMovementComponent& MoveComponent) override;
};
