// Fill out your copyright notice in the Description page of Project Settings.


#include "ACTGameInstance.h"
#include "Asset/ACTAssetManager.h"
#include "Client/Log/UELog.h"
#include "Framework/ACTGameGameInstance.h"
#include "Framework/Misc/ACTGameMemory.h"

void UACTGameInstance::Init()
{
    //////logic gameinstance//////
    auto& GameInstance = ACTGame::ACTGameInstance::Get();
    ACT_UELog* UELog = ACTGameGlobal::NewObject<ACT_UELog>();
    std::unique_ptr<ACTGame::ILogService, ACTGameGlobal::DefaultDeleter> UELogPtr(UELog);
    GameInstance.SetLogger(std::move(UELogPtr));
    GameInstance.Init();
    //////logic gameinstance//////
    
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

void UACTGameInstance::Shutdown()
{
    auto& GameInstance = ACTGame::ACTGameInstance::Get();
    GameInstance.Shutdown();
    Super::Shutdown();
}
