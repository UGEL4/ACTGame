// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Commands/Commands.h"
#include "ActionEditorStyle.h"

class FActionEditorCommands : public TCommands<FActionEditorCommands>
{
public:

	FActionEditorCommands()
		: TCommands<FActionEditorCommands>(TEXT("ActionEditor"), NSLOCTEXT("Contexts", "ActionEditor", "ActionEditor Plugin"), NAME_None, FActionEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};