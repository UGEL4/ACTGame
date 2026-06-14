#pragma once

#include "CoreMinimal.h"
#include "HAL/Platform.h"
#include "UObject/ObjectMacros.h"
#include "PreorderActionInfo.generated.h"

USTRUCT(BlueprintType)
struct FPreorderActionInfo
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ActionId;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Priority = 0;

    FPreorderActionInfo() = default;
    FPreorderActionInfo(FName InActionId, int32 InPriority)
        : ActionId(InActionId), Priority(InPriority)
    {
    }
};
