// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Asset/ACTGameDataAssetBase.h"
#include "Framework/Game/Command/ActionCommand.h"
#include "GameplayTagContainer.h"
#include "RootMotionAsset.h"
#include "UObject/ObjectMacros.h"
#include "Framework/Game/Action/CancelTag.h"
#include "ActionInfoAsset.generated.h"

USTRUCT(BlueprintType)
struct FActionFrameData
{
	GENERATED_BODY()

	// UPROPERTY(EditAnywhere, BlueprintReadOnly)
	// FGameplayTagContainer CancelTags;
};

/**
 * 
 */
UCLASS(BlueprintType)
class ACTGAME_API UActionInfoAsset : public UACTGameDataAssetBase
{
	GENERATED_BODY()
	
	
public:
	UActionInfoAsset();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FName ActionName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TArray<FActionCommand> Commands;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FCancelTag> CancelTags;

    // 这个动作可以被哪些动作cancel
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FBeCancelledTag> BeCancelledTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 FrameNum = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName AutoNextActionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool KeepPlayingAnimation = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool AutoTerminate = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    URootMotionAsset* RootMotionAsset;
};
