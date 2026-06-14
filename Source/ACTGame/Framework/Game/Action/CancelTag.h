#pragma once

#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "FrameRange.h"
#include "HAL/Platform.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "CancelTag.generated.h"

USTRUCT(BlueprintType)
struct FCancelTag
{
    GENERATED_BODY()

    // id
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Tag;

    // cancel动作时，优先级变化
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Priority = 0;
};

USTRUCT(BlueprintType)
struct FBeCancelledTag
{
    GENERATED_BODY()

    // 生效的区间
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FFrameRange FrameRange;

    // ids 可以Cancel的CancelTag的列表
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> CancelTags;

    // 被cancel动作时，优先级变化
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Priority = 0;
};