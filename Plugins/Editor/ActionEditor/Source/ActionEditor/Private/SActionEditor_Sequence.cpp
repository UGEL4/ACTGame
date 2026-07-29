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
#include "../Section/Section_CancelTag.h"
#include "../Section/Section_ActionInfo.h"
#include "../Section/ActionCommand/Section_Command.h"
#include "Framework/Game/Action/ActionInfo.h"
#include "Asset/ActionInfo/ActionInfoAsset.h"
#include "Factories/DataAssetFactory.h"
#include "UObject/SavePackage.h"
#include "Misc/MessageDialog.h"
#include "DesktopPlatformModule.h"
#include "FileHelpers.h"
#include "MovieSceneTimeHelpers.h"
#include "EditorDirectories.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "../TrackEditor/ActionInfo_TrackEditor.h"

#define LOCTEXT_NAMESPACE "SActionEditor_Sequence"

SActionEditor_Sequence::~SActionEditor_Sequence()
{
    // Sequencer.Reset();
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
    FString PackagePath     = "/Game/ActionEditor/";
    FString SequenceName    = "MyCreatedSequence";
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
        InitParams.RootSequence           = CurrentLevelSequence; // 关联你的序列资产
        InitParams.bEditWithinLevelEditor = false;                // 关键：设为false，表示它不依赖于主关卡编辑器
        InitParams.ToolkitHost            = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor").GetFirstLevelEditor();
        // ... 根据需要设置其他参数，如回调等
        InitParams.PlaybackContext.BindLambda([]() -> UObject*
                                              { return GEditor->GetEditorWorldContext().World(); });
        InitParams.ViewParams.UniqueName                     = "ActionEditor";
        InitParams.ViewParams.ScrubberStyle                  = ESequencerScrubberStyle::FrameBlock;
        InitParams.ViewParams.bShowPlaybackRangeInTimeSlider = true;

        InitParams.HostCapabilities.bSupportsCurveEditor           = true;
        InitParams.HostCapabilities.bSupportsSaveMovieSceneAsset   = true;
        InitParams.HostCapabilities.bSupportsRecording             = true;
        InitParams.HostCapabilities.bSupportsRenderMovie           = true;
        InitParams.HostCapabilities.bSupportsAddFromContentBrowser = true;
        InitParams.HostCapabilities.bSupportsSidebar               = true;
        InitParams.HostCapabilities.bSupportsViewportSelectability = true;

        // 4. 创建SSequencer实例
        Sequencer = Module.CreateSequencer(InitParams);
    }

    // CurrentLevelSequence = NewObject<ULevelSequence>(GetTransientPackage(), NAME_None, RF_Transactional);
    ChildSlot
    [
    // Populate the widget
    // SNew(SVerticalBox)
    // + SVerticalBox::Slot()
    // .AutoHeight()
    // [
    // ]
    Sequencer->GetSequencerWidget()];
}

void SActionEditor_Sequence::SaveActionAsset()
{
    if (!Sequencer.IsValid())
    {
        return;
    }

    CreateOrEditActionInfoAssetWithDialog([this](UActionInfoAsset* ActionAsset) -> bool
                                          {
		UMovieScene* MovieScene = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
		if (!MovieScene)
		{
			return false;
		}

		UTrack_ActionInfo* ActionInfoTrak{ nullptr };
		UTrack_CancelTag* CancelTagTrack{ nullptr };
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
				if (!CancelTagTrack)
				{
					CancelTagTrack = Cast<UTrack_CancelTag>(Track);
				}
			}
			else if (Track->IsA<UTrack_Command>())
			{
				if (!CommandTrack)
				{
					CommandTrack = Cast<UTrack_Command>(Track);
				}
			}
		}
		if (!ActionInfoTrak)
		{
			return false;
		}

		auto& ActionSections = ActionInfoTrak->GetAllSections();
		if (ActionSections.Num() == 0)
		{
			return false;
		}
        auto StartFrame  = GetStartFrame(ActionSections[0]);
        auto EndFrame    = GetEndFrame(ActionSections[0]);
        auto FrameNumber = EndFrame - StartFrame + 1;
        TArray<FActionFrame> ActionFrames;
        ActionFrames.SetNum(FrameNumber);
        if (CancelTagTrack)
        {
            for (auto Section : CancelTagTrack->GetAllSections())
            {
                int32 Start = GetStartFrame(Section);
                int32 End   = GetEndFrame(Section);
                if (End >= ActionFrames.Num())
                {
                    continue;
                }
                auto CancelTagSection = Cast<USection_CancelTag>(Section);
                for (int32 i = Start; i <= End; i++)
                {
                    ActionFrames[i].CancelTags = CancelTagSection->TagList;
                }
            }
        }
        auto ActionInfoSection      = Cast<USection_ActionInfo>(ActionSections[0]);
        ActionAsset->ActionName     = ActionInfoSection->ActionName;
        ActionAsset->FrameList      = ActionFrames;
        ActionAsset->CancelDataList = ActionInfoSection->CancelDataList;
        if (CommandTrack && CommandTrack->GetAllSections().Num() > 0)
        {
            auto CommandSection   = Cast<USection_Command>(CommandTrack->GetAllSections()[0]);
            ActionAsset->Commands = CommandSection->CommandList;
        }
        return true; });

    /*/ 1. 获取 AssetTools 模块
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    IAssetTools& AssetTools = AssetToolsModule.Get();

    // 2. 定义资产路径和名称
    //FString AssetName = TEXT("MyNewAsset");
    FString BasePackagePath = TEXT("/Game/Asset/ActionInfo/MyNewAsset"); // 在 /Game/Asset/ActionInfo 目录下
    FString PackageName;
    FString AssetName;
    // Generates a safe, non-conflicting path (e.g., MyNewAsset_1 if MyNewAsset exists)
    AssetToolsModule.Get().CreateUniqueAssetName(BasePackagePath, TEXT(""), PackageName, AssetName);
    FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

    // 3. 创建 UDataAssetFactory 并指定你要创建的类
    UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
    Factory->DataAssetClass = UActionInfoAsset::StaticClass(); // 关键步骤：指定为你的类

    // 4. 使用 AssetTools 创建资产
    // CreateAsset 的参数：资产名称, 包路径, 资产类, 工厂
    UObject* NewAsset = AssetTools.CreateAsset(AssetName, PackagePath, UActionInfoAsset::StaticClass(), Factory);

    if (UActionInfoAsset* ActionAsset = Cast<UActionInfoAsset>(NewAsset))
    {
        auto ActionInfoSection = Cast<USection_ActionInfo>(ActionSections[0]);
        // 5. 设置你的数据
        ActionAsset->ActionName = ActionInfoSection->ActionName;
        ActionAsset->FrameList  = ActionFrames;
        ActionAsset->CancelDataList = ActionInfoSection->CancelDataList;
        if (CommandTrack->GetAllSections().Num() > 0)
        {
            auto CommandSection = Cast<USection_Command>(CommandTrack->GetAllSections()[0]);
            ActionAsset->Commands = CommandSection->CommandList;
        }

        // Notify the Asset Registry that a new asset exists in the content browser
        FAssetRegistryModule::AssetCreated(NewAsset);
        // 6. 标记为脏并保存
        // AssetTools.CreateAsset 会自动标记包为脏并保存，但为安全起见，可以再次确认
        UPackage* Package = ActionAsset->GetPackage();
        Package->MarkPackageDirty();

        FSavePackageArgs SaveArgs;
        SaveArgs.SaveFlags = RF_Public | RF_Standalone;

        FString PackageFileName = FPackageName::LongPackageNameToFilename(
            PackageName,
            FPackageName::GetAssetPackageExtension()
        );

        UPackage::Save(Package, NewAsset, *PackageFileName, SaveArgs);
    }*/
}

bool SActionEditor_Sequence::CreateOrEditActionInfoAssetWithDialog(TFunction<bool(UActionInfoAsset*)> ModifyFunc)
{
    /*
    // ---------- 1. 打开保存对话框 ----------
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (!DesktopPlatform)
        return false;

    // 设置默认路径为 Content/Asset/ActionInfo（您可以根据需要修改）
    FString DefaultPath = FPaths::ProjectContentDir() / TEXT("Asset/ActionInfo");
    FString DefaultFile = TEXT("NewActionInfo");
    FString FileTypes = TEXT("Unreal Asset File (*.uasset)|*.uasset|All Files (*.*)|*.*");
    TArray<FString> OutFilenames;

    bool bOpened = DesktopPlatform->SaveFileDialog(
        FSlateApplication::Get().GetActiveTopLevelWindow()->GetNativeWindow()->GetOSWindowHandle(),
        TEXT("保存动作信息资产"),
        DefaultPath,
        DefaultFile,
        FileTypes,
        EFileDialogFlags::None,
        OutFilenames
    );

    if (!bOpened || OutFilenames.Num() == 0)
        return false;   // 用户取消

    FString FilePath = OutFilenames[0];

    // 确保文件有 .uasset 扩展名（如果用户没有输入）
    if (!FilePath.EndsWith(TEXT(".uasset")))
        FilePath += TEXT(".uasset");

    // ---------- 2. 将物理路径转换为包路径 ----------
    FString PackageName;
    if (!FPackageName::TryConvertFilenameToLongPackageName(FilePath, PackageName))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("选择的路径不在 Content 目录内，请选择有效资产位置。")));
        return false;
    }

    FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
    FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

    // ---------- 3. 检查资产是否已存在 ----------
    UActionInfoAsset* ExistingAsset = LoadObject<UActionInfoAsset>(nullptr, *PackageName);

    if (ExistingAsset)
    {
        // 资产已存在 -> 询问是否覆盖
        EAppReturnType::Type Result = FMessageDialog::Open(
            EAppMsgType::YesNo,
            FText::FromString(FString::Printf(TEXT("资产 “%s” 已存在，是否覆盖并修改？"), *AssetName)),
            FText::FromString(TEXT("资产已存在"))
        );

        if (Result == EAppReturnType::No)
            return false;   // 用户取消操作
    }
    // ---------- 4. 创建或获取资产对象（但不保存） ----------
    UActionInfoAsset* TargetAsset = ExistingAsset;
    UPackage* Package = nullptr;

    if (!ExistingAsset)
    {
        // 创建新的 Package
        Package = CreatePackage(*PackageName);
        Package->SetFlags(RF_Public | RF_Standalone);

        // 创建对象，Outer 为 Package
        TargetAsset = NewObject<UActionInfoAsset>(
            Package,
            UActionInfoAsset::StaticClass(),
            *AssetName,
            RF_Public | RF_Standalone | RF_Transactional
        );
        if (!TargetAsset)
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("创建资产对象失败。")));
            return false;
        }
    }
    else
    {
        // 若已存在，直接使用加载的对象（此时它可能已有数据，但我们将覆盖）
        Package = ExistingAsset->GetPackage();
        // 注意：加载的对象可能已经标记为脏，但我们将修改并重新保存
    }

    // ---------- 5. 执行修改回调 ----------
    bool bModifySuccess = ModifyFunc(TargetAsset);
    if (!bModifySuccess)
    {
        // 修改失败：如果是新建对象，直接丢弃；如果是已有对象，不保存修改
        if (!ExistingAsset)
        {
            // 清理：标记包为“垃圾”，并清理对象引用（可选）
            // 简单做法：不保存，对象会在编辑器关闭或GC时被回收
            // 但为稳妥，可以标记包为“不保存”
            Package->ClearFlags(RF_Public | RF_Standalone); // 移除持久化标记
            // 通知 Asset Registry 移除（如果有）
            FAssetRegistryModule::AssetDeleted(TargetAsset);
        }
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("修改资产失败，操作已取消。")));
        return false;
    }

    // ---------- 6. 保存资产 ----------
    Package->MarkPackageDirty();

    // 如果是新建资产，需要通知 Asset Registry
    if (!ExistingAsset)
    {
        FAssetRegistryModule::AssetCreated(TargetAsset);
    }

    // 使用 FEditorFileUtils 保存（支持版本控制）
    TArray<UPackage*> PackagesToSave;
    PackagesToSave.Add(Package);
    auto Result = FEditorFileUtils::PromptForCheckoutAndSave(
        PackagesToSave,
        true,
        false   // 静默保存，不弹额外对话框
    );

    if (Result != FEditorFileUtils::EPromptReturnCode::PR_Success)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("保存资产失败。")));
        return false;
    }

    return true; */

    // 1. 配置保存对话框
    FSaveAssetDialogConfig Config;
    {
        Config.DialogTitleOverride = LOCTEXT("SaveActionInfoAsset", "保存动作信息资产");
        Config.DefaultPath         = TEXT("/Game/Asset/ActionInfo"); // 默认目录
        Config.DefaultAssetName    = TEXT("NewActionInfo");          // 默认文件名
        Config.AssetClassNames.Add(UActionInfoAsset::StaticClass()->GetClassPathName());
        // 当资产已存在时，允许保存但警告（我们会在回调中自行处理覆盖逻辑）
        Config.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::AllowButWarn;
    }

    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

    // 2. 模态打开对话框，阻塞直到用户操作完成
    FString SaveObjectPath = ContentBrowserModule.Get().CreateModalSaveAssetDialog(Config);
    if (SaveObjectPath.IsEmpty())
    {
        // 用户取消
        UE_LOG(LogTemp, Log, TEXT("用户取消了保存操作"));
        return false;
    }

    // ---------- 3. 检查资产是否已存在 ----------
    FString PackageName = FPackageName::ObjectPathToPackageName(SaveObjectPath);
    UActionInfoAsset* ExistingAsset = LoadObject<UActionInfoAsset>(nullptr, *PackageName);
    UActionInfoAsset* TargetAsset   = nullptr;
    UPackage* Package               = nullptr;

    if (ExistingAsset)
    {
        // 资产已存在，询问是否覆盖
        // EAppReturnType::Type Result = FMessageDialog::Open(
        // EAppMsgType::YesNo,
        // FText::FromString(FString::Printf(TEXT("资产 “%s” 已存在，是否覆盖并修改？"), *FPackageName::GetLongPackageAssetName(PackageName))),
        // FText::FromString(TEXT("资产已存在")));
        // if (Result == EAppReturnType::No)
        // {
        //     // 用户取消，直接返回
        //     return false;
        // }
        // // 用户确认覆盖，继续使用 ExistingAsset
        TargetAsset = ExistingAsset;
        Package = ExistingAsset->GetPackage();
    }
    else
    {
        // ---------- 4. 资产不存在，创建新资产（延迟保存） ----------
        Package = CreatePackage(*PackageName);
        Package->SetFlags(RF_Public | RF_Standalone);

        FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
        TargetAsset       = NewObject<UActionInfoAsset>(Package, UActionInfoAsset::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);

        if (!TargetAsset)
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("创建资产对象失败。")));
            return false;
        }
    }

    // ---------- 5. 执行修改回调 ----------
    bool bModifySuccess = ModifyFunc(TargetAsset);
    if (!bModifySuccess)
    {
        // 修改失败：如果是新建资产，丢弃对象，不留垃圾文件
        if (!ExistingAsset)
        {
            Package->ClearFlags(RF_Public | RF_Standalone);
            FAssetRegistryModule::AssetDeleted(TargetAsset);
        }
        // 如果是已有资产，则放弃修改（不保存）
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("修改资产失败，操作已取消。")));
        return false;
    }

    // ---------- 6. 保存修改 ----------
    Package->MarkPackageDirty();

    if (!ExistingAsset)
    {
        // 新建资产需要通知 Asset Registry，使其在内容浏览器中显示
        FAssetRegistryModule::AssetCreated(TargetAsset);
    }

    // 使用 FEditorFileUtils 保存（自动处理版本控制检出）
    TArray<UPackage*> PackagesToSave;
    PackagesToSave.Add(Package);
    auto Result = FEditorFileUtils::PromptForCheckoutAndSave(
    PackagesToSave,
    true, // 只保存脏包
    false // 静默保存，不额外弹窗
    );

    if (Result != FEditorFileUtils::EPromptReturnCode::PR_Success)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("保存资产失败，请检查磁盘空间或版本控制状态。")));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("资产保存成功: %s"), *PackageName);
    }
    return true;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

int32 SActionEditor_Sequence::GetStartFrame(UMovieSceneSection* Section)
{
    if (!Section)
    {
        FFrame::KismetExecutionMessage(TEXT("Cannot call GetEndFrame on a null section"), ELogVerbosity::Error);
        return -1;
    }

    if (!Section->HasStartFrame())
    {
        FFrame::KismetExecutionMessage(TEXT("Section does not have a start frame"), ELogVerbosity::Error);
        return -1;
    }

    UMovieScene* MovieScene = Section->GetTypedOuter<UMovieScene>();
    if (MovieScene)
    {
        FFrameRate DisplayRate = MovieScene->GetDisplayRate();
        return ConvertFrameTime(UE::MovieScene::DiscreteInclusiveLower(Section->GetRange()), MovieScene->GetTickResolution(), DisplayRate).FloorToFrame().Value;
    }
    else
    {
        return -1;
    }
}

int32 SActionEditor_Sequence::GetEndFrame(UMovieSceneSection* Section)
{
    if (!Section)
    {
        FFrame::KismetExecutionMessage(TEXT("Cannot call GetEndFrame on a null section"), ELogVerbosity::Error);
        return -1;
    }

    if (!Section->HasEndFrame())
    {
        FFrame::KismetExecutionMessage(TEXT("Section does not have an end frame"), ELogVerbosity::Error);
        return -1;
    }

    UMovieScene* MovieScene = Section->GetTypedOuter<UMovieScene>();
    if (MovieScene)
    {
        FFrameRate DisplayRate = MovieScene->GetDisplayRate();
        return ConvertFrameTime(UE::MovieScene::DiscreteExclusiveUpper(Section->GetRange()), MovieScene->GetTickResolution(), DisplayRate).FloorToFrame().Value;
    }
    else
    {
        return -1;
    }
}

void SActionEditor_Sequence::OpenActionInfo()
{
    struct FLocal
    {
        static void OnLevelsSelected(const TArray<FAssetData>& SelectedLevels, FEditorFileUtils::FOnLevelsChosen OnLevelsChosenDelegate)
        {
            if (SelectedLevels.Num() > 0)
            {
                // We selected a level. Save the path to this level to use as the default path next time we open.
                const FAssetData& FirstAssetData = SelectedLevels[0];

                // Convert from package name to filename. Add a trailing slash to prevent an invalid conversion when an asset is in a root folder (e.g. /Game)
                FString FilesystemPath = FPackageName::LongPackageNameToFilename(FirstAssetData.PackagePath.ToString() + TEXT("/"));
                ;

                // Remove the slash if needed
                if (FilesystemPath.EndsWith(TEXT("/"), ESearchCase::CaseSensitive))
                {
                    FilesystemPath.LeftChopInline(1, EAllowShrinking::No);
                }

                FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_OPEN, FilesystemPath);

                OnLevelsChosenDelegate.ExecuteIfBound(SelectedLevels);
            }
        }

        static void OnDialogCancelled(FEditorFileUtils::FOnLevelPickingCancelled OnLevelPickingCancelledDelegate)
        {
            OnLevelPickingCancelledDelegate.ExecuteIfBound();
        }
    };

    // Copy from FEditorFileUtils::OpenLevelPickingDialog
    // Determine the starting path. Try to use the most recently used directory
    FString DefaultPath;
    {
        FString DefaultFilesystemDirectory = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_OPEN);

        // ensure trailing "/" for directory name since TryConvertFilenameToLongPackageName expects one
        if (!DefaultFilesystemDirectory.IsEmpty() && !DefaultFilesystemDirectory.EndsWith("/"))
        {
            DefaultFilesystemDirectory.AppendChar(TEXT('/'));
        }

        if (DefaultFilesystemDirectory.IsEmpty() || !FPackageName::TryConvertFilenameToLongPackageName(DefaultFilesystemDirectory, DefaultPath))
        {
            // No saved path, just use a reasonable default
            DefaultPath = TEXT("/Game/Asset/ActionInfo");
        }

        // OpenAssetDialog expects no trailing "/" so remove if necessary
        DefaultPath.RemoveFromEnd(TEXT("/"));
    }

    FOpenAssetDialogConfig OpenAssetDialogConfig;
    OpenAssetDialogConfig.DialogTitleOverride = LOCTEXT("OpenActionInfoDialogTitle", "Open ActionInfo");
    OpenAssetDialogConfig.DefaultPath         = DefaultPath;
    OpenAssetDialogConfig.AssetClassNames.Add(UActionInfoAsset::StaticClass()->GetClassPathName());
    OpenAssetDialogConfig.bAllowMultipleSelection = false;

    auto OnActionChosenFunc = FEditorFileUtils::FOnLevelsChosen::CreateSP(this, &SActionEditor_Sequence::OnActionChosen);

    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    ContentBrowserModule.Get().CreateOpenAssetDialog(OpenAssetDialogConfig,
                                                     FOnAssetsChosenForOpen::CreateStatic(&FLocal::OnLevelsSelected, OnActionChosenFunc),
                                                     FOnAssetDialogCancelled::CreateStatic(&FLocal::OnDialogCancelled, FEditorFileUtils::FOnLevelPickingCancelled::CreateLambda([]() {})));
}

void SActionEditor_Sequence::OnActionChosen(const TArray<FAssetData>& Assets)
{
    if (Assets.Num() == 0)
    {
        return;
    }
    UObject* Obj = Assets[0].GetAsset();
    if (!Obj)
    {
        return;
    }
    if (UActionInfoAsset* ActionAsset = Cast<UActionInfoAsset>(Obj))
    {
        UE_LOG(LogTemp, Warning, TEXT("已加载："), *ActionAsset->ActionName.ToString());
        //ActionInfoAsset = ActionAsset;
        // 构造sequence
        UMovieScene* MovieScene = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
        if (!MovieScene)
        {
            return;
        }
        // 清理旧的数据
        auto& Tracks = MovieScene->GetTracks();
        for (int32 i = Tracks.Num() - 1; i >= 0; i--)
        {
            MovieScene->RemoveTrack(*Tracks[i]);
        }
        // UTrack_ActionInfo
        //Sequencer->GetTrackEditor()
        UTrack_ActionInfo* NewTrack = BuildActionInfoTrack(*ActionAsset);
    }
}

UTrack_ActionInfo* SActionEditor_Sequence::BuildActionInfoTrack(const UActionInfoAsset& ActionAsset)
{
    UMovieScene* MovieScene     = Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene();
    UTrack_ActionInfo* NewTrack = nullptr;
    {
        const FScopedTransaction Transaction(LOCTEXT("ActionEditorSequence_Transaction", "Add ActionInfo Track"));
        MovieScene->Modify();

        NewTrack = MovieScene->AddTrack<UTrack_ActionInfo>();
        check(NewTrack);

        NewTrack->SetDisplayName(LOCTEXT("ActionInfoTrackName", "ActionInfo"));

        if (Sequencer.IsValid())
        {
            Sequencer->OnAddTrack(NewTrack, FGuid());
        }
    }
    int32 FrameNum = ActionAsset.FrameList.Num();
    if (FrameNum == 0)
    {
        FrameNum = 10;
    }

    FFrameRate DisplayRate          = Sequencer->GetFocusedDisplayRate();
    FFrameRate TickResolution       = Sequencer->GetFocusedTickResolution();
    FQualifiedFrameTime CurrentTime = Sequencer->GetLocalTime();
    FFrameNumber PlaybackEnd        = UE::MovieScene::DiscreteExclusiveUpper(Sequencer->GetFocusedMovieSceneSequence()->GetMovieScene()->GetPlaybackRange());

    int32 SpecifiedRowIndex = 0;

    FScopedTransaction Transaction(LOCTEXT("AddSectionTransactionText", "Add Section"));
    if (UMovieSceneSection* NewSection = NewTrack->CreateNewSection())
    {
        USection_ActionInfo* S = Cast<USection_ActionInfo>(NewSection);
        {
            S->ActionName           = ActionAsset.ActionName;
            S->CancelDataList       = ActionAsset.CancelDataList;
            S->AutoNextActionId     = ActionAsset.AutoNextActionId;
            S->KeepPlayingAnimation = ActionAsset.KeepPlayingAnimation;
            S->AutoTerminate        = ActionAsset.AutoTerminate;
            S->Priority             = FrameNum;
        }

        int32 OverlapPriority = 0;
        TMap<int32, int32> NewToOldRowIndices;

        NewTrack->Modify();
        NewTrack->OnRowIndicesChanged(NewToOldRowIndices);
        /* int32 a = DisplayRate.Numerator;
        int32 b = TickResolution.Numerator;
        int32 e = FrameNum / a * b;
        UE_LOG(LogTemp, Warning, TEXT(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>：%d, %d, %d"), a, b, e);*/

        FFrameTime SourceTime = FFrameTime::FromDecimal(FrameNum);
        FFrameTime EndTime    = ConvertFrameTime(SourceTime, DisplayRate, TickResolution);
        NewSection->SetRange(TRange<FFrameNumber>(FFrameNumber(0), EndTime.FrameNumber));

        NewSection->SetOverlapPriority(OverlapPriority);
        NewSection->SetRowIndex(SpecifiedRowIndex);
        NewSection->SetBlendType(EMovieSceneBlendType::Absolute);

        NewTrack->AddSection(*NewSection);
        NewTrack->UpdateEasing();

        if (UMovieSceneNameableTrack* NameableTrack = Cast<UMovieSceneNameableTrack>(NewTrack))
        {
            NameableTrack->SetTrackRowDisplayName(FText::GetEmpty(), SpecifiedRowIndex);
        }

        Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
    }
    else
    {
        Transaction.Cancel();
    }
    return NewTrack;
}

#undef LOCTEXT_NAMESPACE
