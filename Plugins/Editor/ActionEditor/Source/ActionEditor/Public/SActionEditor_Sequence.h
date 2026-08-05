// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ISequencerModule.h"

/**
 * 
 */

class UTrack_ActionInfo;
class UActionInfoAsset;
class UTrack_CancelTag;
class UTrack_HitBox;

class ACTIONEDITOR_API SActionEditor_Sequence : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SActionEditor_Sequence)
	{}
	SLATE_END_ARGS()

	~SActionEditor_Sequence();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	void OnTabClosed(TSharedRef<SDockTab> ClosedTab);

	void SaveActionAsset();

private:
	bool CreateOrEditActionInfoAssetWithDialog(TFunction<bool(class UActionInfoAsset*)> ModifyFunc);
    int32 GetStartFrame(UMovieSceneSection* Section);
    int32 GetEndFrame(UMovieSceneSection* Section);

public:
    void OpenActionInfo();
    void ClearInvalidHitBoxActor();

private:
    void OnActionChosen(const TArray<FAssetData>& Assets);
    UTrack_ActionInfo* BuildActionInfoTrack(const UActionInfoAsset& ActionAsset);
    UTrack_CancelTag* BuildCancelTagTrack(const UActionInfoAsset& ActionAsset);
    UTrack_HitBox* BuildHitBoxTrack(const UActionInfoAsset& ActionAsset);

private:
	UPROPERTY()
	class ULevelSequence* CurrentLevelSequence{ nullptr };

	TSharedPtr<ISequencer> Sequencer;

    UPROPERTY()
    TObjectPtr<UActionInfoAsset> ActionInfoAsset{ nullptr };
};
