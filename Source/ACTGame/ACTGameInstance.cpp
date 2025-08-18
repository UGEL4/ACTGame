// Fill out your copyright notice in the Description page of Project Settings.


#include "ACTGameInstance.h"
#include "Asset/ACTAssetManager.h"

void UACTGameInstance::Init()
{
	Super::Init();

	UACTAssetManager& mgr = UACTAssetManager::Get();
	for (auto type : AssetTypeList)
	{
		TArray<FPrimaryAssetId> idList;
		if (mgr.GetPrimaryAssetIdList(type, idList))
		{
			mgr.LoadPrimaryAssets(idList, TArray<FName>(), FStreamableDelegate(), 0);
		}
	}
}
