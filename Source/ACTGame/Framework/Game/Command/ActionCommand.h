// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActionCommand.generated.h"


UENUM(BlueprintType)
enum class EKeyMap : uint8
{
	NoDir,
	NoInput,
	DirInput,
	ButtonWest,
	ButtonEast,
	ButtonNouth,
	ButtonSouth
};

/**
 * 
 */
USTRUCT(BlueprintType)
struct FActionCommand
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EKeyMap> keySequences;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ValidInFrameCount;
};
