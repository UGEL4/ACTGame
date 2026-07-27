#pragma once

#include "CancelTag.h"
#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "Framework/Game/Command/ActionCommand.h"
#include "Asset/ActionInfo/RootMotionAsset.h"
#include "ActionInfo.generated.h"

USTRUCT(BlueprintType)
struct ACTGAME_API FActionFrame
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float AnimKeyFrame{ 0.0 };

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int LoopFrame{ 1 };

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FCancelTag> CancelTags;
    // 攻击框
    // 受击框
};

USTRUCT(BlueprintType)
struct FActionInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FActionCommand> Commands;

    // 这个动作可以cancel那些动作
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FCancelTag> CancelTags;

    // 这个动作可以被哪些动作cancel
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FBeCancelledTag> BeCancelledTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 FrameNum = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AutoNextActionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool KeepPlayingAnimation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool AutoTerminate = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    URootMotionAsset* RootMotionAsset;
};
