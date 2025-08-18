// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "ACTAssetManager.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogACTAssetManager, Log, All);
/**
 * 
 */
UCLASS()
class ACTGAME_API UACTAssetManager : public UAssetManager
{
	GENERATED_BODY()
	
public:
	UACTAssetManager();
	
	static UACTAssetManager& Get();

	static const FPrimaryAssetType ActionInfo;
};
