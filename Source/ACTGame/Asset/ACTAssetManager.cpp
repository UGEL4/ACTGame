// Fill out your copyright notice in the Description page of Project Settings.


#include "Asset/ACTAssetManager.h"

DEFINE_LOG_CATEGORY(LogACTAssetManager);

const FPrimaryAssetType UACTAssetManager::ActionInfo = TEXT("ActionInfo");

UACTAssetManager::UACTAssetManager()
{

}

UACTAssetManager& UACTAssetManager::Get()
{
	UACTAssetManager* Singleton = Cast<UACTAssetManager>(GEngine->AssetManager);

	if (Singleton)
	{
		return *Singleton;
	}
	else
	{
		UE_LOG(LogACTAssetManager, Fatal, TEXT("Cannot use ACTAssetManager if no ACTAssetManagerClassName is defined!"));
		return *NewObject<UACTAssetManager>(); // never calls this
	}
}


