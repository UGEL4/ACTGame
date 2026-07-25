// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MovieSceneNameableTrack.h"
#include "Track_Command.generated.h"

/**
 * 
 */
UCLASS()
class ACTIONEDITOR_API UTrack_Command : public UMovieSceneNameableTrack
{
	GENERATED_BODY()

public:
    UTrack_Command(const FObjectInitializer& InInitializer);
    ~UTrack_Command() {}

public:
    virtual FName GetTrackName() const override;
    virtual void AddSection(UMovieSceneSection& Section) override;
    virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
    virtual UMovieSceneSection* CreateNewSection() override;
    virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
    virtual bool HasSection(const UMovieSceneSection& Section) const override;
    virtual bool IsEmpty() const override;
    virtual void RemoveSection(UMovieSceneSection& Section) override;
    virtual FText GetDefaultDisplayName() const override;
    virtual bool SupportsMultipleRows() const override { return true; }
	
private:
	/** The track's sections. */
	UPROPERTY()
	TArray<TObjectPtr<UMovieSceneSection>> Sections;
};
