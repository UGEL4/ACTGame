// Fill out your copyright notice in the Description page of Project Settings.


#include "Asset/ACTGameDataAssetBase.h"
#include "Asset/ACTAssetManager.h"

UACTGameDataAssetBase::UACTGameDataAssetBase()
{

}

FPrimaryAssetId UACTGameDataAssetBase::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
