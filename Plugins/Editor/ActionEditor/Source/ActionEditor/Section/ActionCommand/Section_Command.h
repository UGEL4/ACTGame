// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneSection.h"
#include "Framework/Game/Command/ActionCommand.h"
#include "Section_Command.generated.h"

/**
 * 
 */
UCLASS()
class ACTIONEDITOR_API USection_Command : public UMovieSceneSection
{
	GENERATED_BODY()
	
public:
    USection_Command(const FObjectInitializer& ObjectInitializer);
    virtual ~USection_Command();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FActionCommand> CommandList;

public:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	
};
