// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Asset/ACTGameDataAssetBase.h"
#include "RootMotionAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class ACTGAME_API URootMotionAsset : public UACTGameDataAssetBase
{
	GENERATED_BODY()
	
	
public:
	URootMotionAsset();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	float SampleFrameRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	TArray<FTransform> RootMotionDeltas;
};
