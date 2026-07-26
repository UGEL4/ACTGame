// Fill out your copyright notice in the Description page of Project Settings.


#include "SActionEditor_Sequence.h"

#include "SlateOptMacros.h"
#include "Modules/ModuleManager.h"
#include "LevelSequence.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "LevelEditor.h"
#include "MovieScene.h"
#include "../Track/Track_ActionInfo.h"
#include "../Track/Track_CancelTag.h"
#include "../Track/ActionCommand/Track_Command.h"

SActionEditor_Sequence::~SActionEditor_Sequence()
{
	//Sequencer.Reset();
	if (Sequencer.IsValid())
	{
		Sequencer->Close();
		Sequencer.Reset();
	}
	CurrentLevelSequence = nullptr;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SActionEditor_Sequence::Construct(const FArguments& InArgs)
{
	ISequencerModule& Module = FModuleManager::LoadModuleChecked<ISequencerModule>("sequencer");

	// 1. Define the package path and name
	FString PackagePath = "/Game/ActionEditor/";
	FString SequenceName = "MyCreatedSequence";
	FString FullPackagePath = PackagePath + SequenceName;

	// 2. Create the Package and Asset
	/*UPackage* Package = CreatePackage(*FullPackagePath);
	Package->SetFlags(RF_Public | RF_Standalone);

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	CurrentLevelSequence = Cast<ULevelSequence>(AssetTools.CreateAsset(
		SequenceName,
		PackagePath,
		ULevelSequence::StaticClass(),
		nullptr
	));*/

	CurrentLevelSequence = NewObject<ULevelSequence>(GetTransientPackage(), NAME_None, RF_Transactional);
	if (CurrentLevelSequence)
	{
		CurrentLevelSequence->Initialize();
		// CurrentLevelSequence->MarkPackageDirty();
		// FAssetRegistryModule::AssetCreated(CurrentLevelSequence);

		// 4. Open the new sequence in the Level Sequence Editor
		/*TSharedPtr<FLevelSequenceEditorToolkit> LevelSequenceEditor = MakeShareable(new FLevelSequenceEditorToolkit());
		LevelSequenceEditor->InitLevelSequenceEditor(
			EToolkitMode::Standalone,
			FToolkitManager::Get().FindHostForWorld(GWorld),
			NewSequence
		);*/

		// 3. 初始化Sequencer的参数
		FSequencerInitParams InitParams;
		InitParams.RootSequence = CurrentLevelSequence; // 关联你的序列资产
		InitParams.bEditWithinLevelEditor = false; // 关键：设为false，表示它不依赖于主关卡编辑器
        InitParams.ToolkitHost            = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor").GetFirstLevelEditor();
		// ... 根据需要设置其他参数，如回调等
		InitParams.PlaybackContext.BindLambda([]() -> UObject* {
			return GEditor->GetEditorWorldContext().World();
		});
		InitParams.ViewParams.UniqueName = "ActionEditor";
		InitParams.ViewParams.ScrubberStyle = ESequencerScrubberStyle::FrameBlock;
		InitParams.ViewParams.bShowPlaybackRangeInTimeSlider = true;

		InitParams.HostCapabilities.bSupportsCurveEditor = true;
		InitParams.HostCapabilities.bSupportsSaveMovieSceneAsset = true;
		InitParams.HostCapabilities.bSupportsRecording = true;
		InitParams.HostCapabilities.bSupportsRenderMovie = true;
		InitParams.HostCapabilities.bSupportsAddFromContentBrowser = true;
		InitParams.HostCapabilities.bSupportsSidebar = true;
		InitParams.HostCapabilities.bSupportsViewportSelectability = true;

		// 4. 创建SSequencer实例
		Sequencer = Module.CreateSequencer(InitParams);
	}

	//CurrentLevelSequence = NewObject<ULevelSequence>(GetTransientPackage(), NAME_None, RF_Transactional);
	ChildSlot
	[
		// Populate the widget
		// SNew(SVerticalBox)
		// + SVerticalBox::Slot()
		// .AutoHeight()
		// [
		// ]
		Sequencer->GetSequencerWidget()
	];
	
}

void SActionEditor_Sequence::SaveActionAsset()
{
    if (!Sequencer.IsValid())
    {
        return;
    }

    UMovieScene* MovieScene = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
    if (!MovieScene)
    {
        return;
    }

    UTrack_ActionInfo* ActionInfoTrak{ nullptr };
    TArray<UTrack_CancelTag*> CancelTagTracks;
    UTrack_Command* CommandTrack{ nullptr };
    auto& Tracks = MovieScene->GetTracks();
    for (auto Track : Tracks)
    {
        if (Track->IsA<UTrack_ActionInfo>())
        {
            if (!ActionInfoTrak)
            {
                ActionInfoTrak = Cast<UTrack_ActionInfo>(Track);
            }
        }
        else if (Track->IsA<UTrack_CancelTag>())
        {
            CancelTagTracks.Add(Cast<UTrack_CancelTag>(Track));
        }
        else if (Track->IsA<UTrack_Command>())
        {
            if (!CommandTrack)
            {
                CommandTrack = Cast<UTrack_Command>(Track);
            }
        }
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
