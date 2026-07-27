// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneSection.h"
#include "Framework/Game/Action/CancelTag.h"
#include "Section_CancelTag.generated.h"

/**
 * 
 */
UCLASS()
class ACTIONEDITOR_API USection_CancelTag : public UMovieSceneSection
{
	GENERATED_BODY()
	
public:
    USection_CancelTag(const FObjectInitializer& ObjectInitializer);
    virtual ~USection_CancelTag();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FCancelTag> TagList;

public:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	
};
