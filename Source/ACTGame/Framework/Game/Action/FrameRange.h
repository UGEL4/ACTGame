#pragma once

/**
 * Frame range structure for defining a range of frames
 */

#include "CoreMinimal.h"
#include "HAL/Platform.h"
#include "UObject/ObjectMacros.h"
#include "FrameRange.generated.h"

USTRUCT(BlueprintType)
struct FFrameRange
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Min = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Max = 0;
};