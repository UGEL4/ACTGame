// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Asset/ACTGameDataAssetBase.h"
#include "Framework/Game/Command/ActionCommand.h"
#include "ActionInfoAsset.generated.h"

/**
 * 
 */
UCLASS()
class ACTGAME_API UActionInfoAsset : public UACTGameDataAssetBase
{
	GENERATED_BODY()
	
	
public:
	UActionInfoAsset();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	FName ActionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	TArray<FActionCommand> Commands;
};
