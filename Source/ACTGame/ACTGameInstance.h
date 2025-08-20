// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ACTGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class ACTGAME_API UACTGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
	
public:
	virtual void Init() override;
    virtual void Shutdown() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionType")
	TArray<FPrimaryAssetType> AssetTypeList;
	
};

