// Fill out your copyright notice in the Description page of Project Settings.


#include "SActionEditorMenu.h"

#include "SlateOptMacros.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "SActionEditor_Sequence.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SActionEditorMenu::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SHorizontalBox)
				//+ SHorizontalBox::Slot()
				//.VAlign(VAlign_Top)
				//[
				//	SNew(STextBlock).Text(FText::FromString(TEXT("打开关卡")))
				//]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SButton).Text(FText::FromString(TEXT("打开关卡")))
					.OnClicked(FOnClicked::CreateSP(this, &SActionEditorMenu::OnClickOpenEditorLevel))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SButton).Text(FText::FromString(TEXT("保存")))
					.OnClicked(FOnClicked::CreateSP(this, &SActionEditorMenu::SaveActionAsset))
				]
                + SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SButton).Text(FText::FromString(TEXT("打开")))
					.OnClicked(FOnClicked::CreateSP(this, &SActionEditorMenu::OpenActionAsset))
				]
			]
		]
	];
}

FReply SActionEditorMenu::OnClickOpenEditorLevel()
{
	UE_LOG(LogTemp, Warning, TEXT("Hello, world"));
	//FEditorFileUtils::OpenLevelPickingDialog(FOnLevelsChosen)
	 // 定义选择关卡后的处理逻辑
	auto OnLevelsChosenCallback = [](const TArray<FAssetData>& ChosenLevels)
		{
			for (const FAssetData& Level : ChosenLevels)
			{
				UE_LOG(LogTemp, Warning, TEXT("选择了关卡: %s"), *Level.AssetName.ToString());
				// 在这里处理你选中的关卡路径
			}
		};

	// 定义取消选择的处理逻辑
	auto OnCancelledCallback = []()
		{
			UE_LOG(LogTemp, Warning, TEXT("关卡选择已取消"));
		};
	FEditorFileUtils::OpenLevelPickingDialog(FEditorFileUtils::FOnLevelsChosen::CreateLambda(OnLevelsChosenCallback), 
		FEditorFileUtils::FOnLevelPickingCancelled::CreateLambda(OnCancelledCallback), 
		false);
	return FReply::Handled();
}

FReply SActionEditorMenu::SaveActionAsset()
{
    TSharedPtr<SActionEditor_Sequence> Ptr = SequenceWidget.Pin();
	if (!Ptr.IsValid())
	{
        return FReply::Handled();
	}

	// 保存数据
    UE_LOG(LogTemp, Warning, TEXT("Save action asset"));
    Ptr->SaveActionAsset();

	return FReply::Handled();
}

FReply SActionEditorMenu::OpenActionAsset()
{
    TSharedPtr<SActionEditor_Sequence> Ptr = SequenceWidget.Pin();
    if (!Ptr.IsValid())
    {
        return FReply::Handled();
    }
    UE_LOG(LogTemp, Warning, TEXT("Open action asset"));
    Ptr->OpenActionInfo();
    return FReply::Handled();
}

void SActionEditorMenu::SetSequencerEditor(TSharedRef<SActionEditor_Sequence> InSequenceWidget)
{
    SequenceWidget = InSequenceWidget;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
