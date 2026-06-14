// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ACTGameDataAssetBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class ACTGAME_API UACTGameDataAssetBase : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UACTGameDataAssetBase();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AssetType")
	FPrimaryAssetType AssetType;
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
