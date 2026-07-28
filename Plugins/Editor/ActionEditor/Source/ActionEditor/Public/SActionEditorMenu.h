// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */

class SActionEditor_Sequence;

class ACTIONEDITOR_API SActionEditorMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SActionEditorMenu)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

public:
	FReply OnClickOpenEditorLevel();
    FReply SaveActionAsset();
    FReply OpenActionAsset();
    void SetSequencerEditor(TSharedRef<SActionEditor_Sequence> InSequenceWidget);

private:
    TWeakPtr<SActionEditor_Sequence> SequenceWidget;
};
