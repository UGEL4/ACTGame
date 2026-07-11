// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestEditorEditorModeToolkit.h"
#include "TestEditorEditorMode.h"
#include "Engine/Selection.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "TestEditorEditorModeToolkit"

FTestEditorEditorModeToolkit::FTestEditorEditorModeToolkit()
{
}

void FTestEditorEditorModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode)
{
	FModeToolkit::Init(InitToolkitHost, InOwningMode);
}

void FTestEditorEditorModeToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(NAME_Default);
}


FName FTestEditorEditorModeToolkit::GetToolkitFName() const
{
	return FName("TestEditorEditorMode");
}

FText FTestEditorEditorModeToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("DisplayName", "TestEditorEditorMode Toolkit");
}

#undef LOCTEXT_NAMESPACE
