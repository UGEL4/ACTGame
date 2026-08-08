// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneSection.h"
#include "Framework/Game/Command/ActionCommand.h"
#include "Section_MoveInputAcceptance.generated.h"

/**
 * 
 */
UCLASS()
class ACTIONEDITOR_API USection_MoveInputAcceptance : public UMovieSceneSection
{
	GENERATED_BODY()
	
public:
    USection_MoveInputAcceptance(const FObjectInitializer& ObjectInitializer);
    ~USection_MoveInputAcceptance();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float InputAcceptance{ 0.0f };

public:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	
};
