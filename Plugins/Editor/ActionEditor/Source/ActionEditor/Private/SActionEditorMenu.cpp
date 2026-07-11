// Fill out your copyright notice in the Description page of Project Settings.


#include "SActionEditorMenu.h"

#include "SlateOptMacros.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"

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
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("打开关卡")))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				[
					SNew(SButton).Text(FText::FromString(TEXT("打开")))
					.OnClicked(FOnClicked::CreateSP(this, &SActionEditorMenu::OnClickOpenEditorLevel))
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

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
