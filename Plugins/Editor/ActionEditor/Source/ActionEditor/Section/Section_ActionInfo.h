// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneSection.h"
#include "ActionInfoDefine.h"
#include "Framework/Game/Action/CancelTag.h"
#include "Section_ActionInfo.generated.h"
/**
 * 
 */
UCLASS()
class ACTIONEDITOR_API USection_ActionInfo : public UMovieSceneSection
{
	GENERATED_BODY()
	
public:
    USection_ActionInfo(const FObjectInitializer& ObjectInitializer);
    virtual ~USection_ActionInfo();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ActionName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FCancelData> CancelDataList;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName AutoNextActionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool KeepPlayingAnimation = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool AutoTerminate = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Priority = 0;

};
