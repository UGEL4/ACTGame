#pragma once

/**
 * Header-only editor helpers for the ALS retarget pipeline (alsretarget.py). Everything the
 * Python API cannot do is collected here and exposed back to Python as unreal.AlsRetargetLibrary.
 *
 * The three walls this gets past:
 *
 *   1. FAnimLinkableElement::LinkedSequence - every montage section and notify holds a hard
 *      pointer to the sequence it is anchored to. Retargeting does not remap it, it is not drawn
 *      anywhere in the montage editor, and it is Read-Only in the Python API. It is however a
 *      real package dependency, which is why the Reference Viewer shows the original ALS
 *      sequence on a montage that appears to reference nothing of the sort.
 *      UAnimMontage::UpdateLinkableElements() re-links them all.
 *
 *   2. UAnimMontage::SlotAnimTracks is BlueprintReadOnly, which is what surfaces as Read-Only in
 *      Python - set_editor_property throws and the pipeline's property walk silently does
 *      nothing for montages. FArchiveReplaceObjectRef works off reflection data and ignores both
 *      Blueprint exposure and C++ access control, so it reaches every UPROPERTY in the graph.
 *      Unlike ConsolidateAssets it does not delete the asset being replaced, so the ALS sources
 *      survive.
 *
 *   3. UObjectRedirector has no Python binding, and unreal.new_object() cannot pass
 *      RF_Public | RF_Standalone even if it did. Authoring one here makes redirector creation
 *      deterministic instead of dependent on the rename heuristics.
 *
 * Ordering note: RepairAnimationAssets replaces references first and re-links second. Doing it
 * the other way round would re-link each element onto whatever its segment still pointed at,
 * pinning the stale reference back in place.
 */

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Engine/Classes/Animation/AnimMontage.h"
#include "Runtime/Engine/Classes/Kismet/BlueprintFunctionLibrary.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "UObject/ObjectRedirector.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Kismet2/KismetEditorUtilities.h"
// #include "AssetToolsModule.h/IAssetTools.h"

#include "AlsRetarget.generated.h"

DEFINE_LOG_CATEGORY_STATIC(LogAlsRetarget, Log, All);

UCLASS()
class ALSSKELETONSEDITOR_API UAlsRetargetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Re-links every section and notify on a montage to the sequence actually sitting in its
	 * segment, clearing LinkedSequence pointers left behind by retargeting.
	 */
	UFUNCTION(BlueprintCallable, Category = "ALS Retarget")
	static bool UpdateMontageLinks(UAnimMontage* Montage)
	{
#if WITH_EDITOR
		if (!IsValid(Montage))
		{
			return false;
		}

		Montage->Modify();
		Montage->UpdateLinkableElements();
		Montage->RefreshCacheData();
		Montage->MarkPackageDirty();
		return true;
#else
		return false;
#endif
	}

	/**
	 * Repoints every reference held by Asset according to ReplacementMap, including UPROPERTYs
	 * that are private, BlueprintReadOnly or otherwise unreachable from Python. Nothing is
	 * deleted. Returns the number of references replaced.
	 */
	UFUNCTION(BlueprintCallable, Category = "ALS Retarget")
	static int32 ReplaceReferences(UObject* Asset, const TMap<UObject*, UObject*>& ReplacementMap)
	{
		if (!IsValid(Asset) || ReplacementMap.Num() == 0)
		{
			return 0;
		}

		Asset->Modify();

		FArchiveReplaceObjectRef<UObject> ReplaceAr(
			Asset,
			ReplacementMap,
			EArchiveReplaceObjectFlags::IgnoreOuterRef | EArchiveReplaceObjectFlags::IgnoreArchetypeRef);

		const int32 NumReplaced = static_cast<int32>(ReplaceAr.GetCount());
		if (NumReplaced > 0)
		{
			Asset->MarkPackageDirty();
#if WITH_EDITOR
			Asset->PostEditChange();
#endif
		}
		return NumReplaced;
	}

	/**
	 * Runs ReplaceReferences over a batch and re-links any montage among them. This is the entry
	 * point the pipeline uses - one call for the whole retargeted set.
	 */
	UFUNCTION(BlueprintCallable, Category = "ALS Retarget")
	static int32 RepairAnimationAssets(const TArray<UObject*>& Assets, const TMap<UObject*, UObject*>& ReplacementMap)
	{
		int32 TotalReplaced = 0;
		for (UObject* Asset : Assets)
		{
			if (!IsValid(Asset))
			{
				continue;
			}

			TotalReplaced += ReplaceReferences(Asset, ReplacementMap);

			if (UAnimMontage* Montage = Cast<UAnimMontage>(Asset))
			{
				UpdateMontageLinks(Montage);
			}
		}
		return TotalReplaced;
	}

	/**
	 * Authors an ObjectRedirector at SourcePackageName pointing at DestinationObject and saves it.
	 * Accepts either a package path (/Game/Foo/Bar) or an object path (/Game/Foo/Bar.Bar).
	 * Refuses to overwrite a real asset; updates the destination of an existing redirector.
	 */
	UFUNCTION(BlueprintCallable, Category = "ALS Retarget")
	static bool CreateRedirector(const FString& SourcePackageName, UObject* DestinationObject)
	{
		if (SourcePackageName.IsEmpty() || !IsValid(DestinationObject))
		{
			return false;
		}

		const FString PackageName = ToPackageName(SourcePackageName);
		FString AssetName;
		if (!PackageName.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::CaseSensitive, ESearchDir::FromEnd)
			|| AssetName.IsEmpty())
		{
			return false;
		}

		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			UE_LOG(LogAlsRetarget, Error, TEXT("Could not create package '%s' for a redirector"), *PackageName);
			return false;
		}
		Package->FullyLoad();

		UObject* Existing = StaticFindObject(UObject::StaticClass(), Package, *AssetName);
		UObjectRedirector* Redirector = Cast<UObjectRedirector>(Existing);

		if (Existing && !Redirector)
		{
			UE_LOG(LogAlsRetarget, Warning,
				TEXT("'%s' already holds a real asset - not replacing it with a redirector"), *PackageName);
			return false;
		}

		if (!Redirector)
		{
			Redirector = NewObject<UObjectRedirector>(Package, *AssetName, RF_Standalone | RF_Public);
			if (!Redirector)
			{
				return false;
			}
			FAssetRegistryModule::AssetCreated(Redirector);
		}

		Redirector->DestinationObject = DestinationObject;
		Redirector->MarkPackageDirty();

		return SavePackageToDisk(Package);
	}

	/**
	 * Renames Asset into DestPackagePath/DestAssetName and guarantees a redirector at the old
	 * path, whatever the rename itself decided to do. Returns the moved asset, or null on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "ALS Retarget")
	static UObject* MoveAssetLeavingRedirector(UObject* Asset, const FString& DestPackagePath, const FString& DestAssetName)
	{
#if WITH_EDITOR
		if (!IsValid(Asset) || DestPackagePath.IsEmpty() || DestAssetName.IsEmpty())
		{
			return nullptr;
		}

		const FString SourcePackageName = Asset->GetOutermost()->GetName();
		if (SourcePackageName == (DestPackagePath / DestAssetName))
		{
			return Asset;
		}

		FAssetToolsModule& AssetToolsModule =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

		TArray<FAssetRenameData> RenameData;
		RenameData.Emplace(Asset, DestPackagePath, DestAssetName);

		if (!AssetToolsModule.Get().RenameAssets(RenameData))
		{
			UE_LOG(LogAlsRetarget, Error, TEXT("Failed to rename '%s' into '%s/%s'"),
				*SourcePackageName, *DestPackagePath, *DestAssetName);
			return nullptr;
		}

		// The engine only leaves a redirector when it judges one warranted; author it regardless
		// so nothing still pointing at the old path can dangle.
		CreateRedirector(SourcePackageName, Asset);
		return Asset;
#else
		return nullptr;
#endif
	}

	/**
	 * The hard package dependencies of an asset, i.e. exactly what the Reference Viewer draws.
	 * Use this to confirm a repaired montage no longer depends on the ALS source animations.
	 */
	UFUNCTION(BlueprintCallable, Category = "ALS Retarget")
	static TArray<FString> GetHardDependencies(const UObject* Asset)
	{
		TArray<FString> Result;
		if (!IsValid(Asset))
		{
			return Result;
		}

		const FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		TArray<FName> Dependencies;
		AssetRegistryModule.Get().GetDependencies(
			FName(*Asset->GetOutermost()->GetName()),
			Dependencies,
			UE::AssetRegistry::EDependencyCategory::Package,
			UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::Hard));

		Result.Reserve(Dependencies.Num());
		for (const FName& Dependency : Dependencies)
		{
			Result.Add(Dependency.ToString());
		}
		return Result;
	}

private:
	/** Strips a trailing .ObjectName off an object path, leaving the package path. */
	static FString ToPackageName(const FString& InPath)
	{
		FString PackageName;
		FString Ignored;
		return InPath.Split(TEXT("."), &PackageName, &Ignored, ESearchCase::CaseSensitive, ESearchDir::FromStart)
			? PackageName
			: InPath;
	}

	static bool SavePackageToDisk(UPackage* Package)
	{
		if (!Package)
		{
			return false;
		}

		const FString FileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		SaveArgs.Error = GError;

		return UPackage::SavePackage(Package, nullptr, *FileName, SaveArgs);
	}

	// Implements a Blueprint Interface on a Blueprint. Caller still compiles the Blueprint afterward.
	UFUNCTION(BlueprintCallable, Category = "AlsRetarget")
	static bool ImplementInterface(UBlueprint* Blueprint, UClass* InterfaceClass)
	{
		if (!Blueprint || !InterfaceClass) return false;
		FBlueprintEditorUtils::ImplementNewInterface(Blueprint, InterfaceClass->GetClassPathName());
		return true;
	}

	// Fixes up referencers of the given redirectors and deletes each once nothing points at it.
	UFUNCTION(BlueprintCallable, Category = "AlsRetarget")
	static void FixupReferencers(const TArray<UObject*>& Redirectors)
	{
		TArray<UObjectRedirector*> Casted;
		for (UObject* Obj : Redirectors)
		{
			if (UObjectRedirector* Redirector = Cast<UObjectRedirector>(Obj))
			{
				Casted.Add(Redirector);
			}
		}
		if (Casted.Num() == 0) return;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().FixupReferencers(Casted, /*bCheckoutDialogPrompt=*/false, ERedirectFixupMode::DeleteFixedUpRedirectors);
	}
};
