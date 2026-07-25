// Fill out your copyright notice in the Description page of Project Settings.


#include "Track_Command.h"
#include "../Section/ActionCommand/Section_Command.h"

#define LOCTEXT_NAMESPACE "UTrack_Command"

UTrack_Command::UTrack_Command(const FObjectInitializer& InInitializer)
    : Super(InInitializer)
{
    SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
#if WITH_EDITORONLY_DATA
     TrackTint = FColor(25, 190, 9, 255);
#endif
 }

FName UTrack_Command::GetTrackName() const
{
    static FName TestTrackName = TEXT("CommandTrack");
    return TestTrackName;
}


void UTrack_Command::AddSection(UMovieSceneSection& Section)
{
    Sections.Add(&Section);
}


bool UTrack_Command::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
    return SectionClass == USection_Command::StaticClass();
}


UMovieSceneSection* UTrack_Command::CreateNewSection()
{
    return NewObject<USection_Command>(this, NAME_None, RF_Transactional);
}

const TArray<UMovieSceneSection*>& UTrack_Command::GetAllSections() const
{
    return Sections;
}

bool UTrack_Command::HasSection(const UMovieSceneSection& Section) const
{
    return Sections.Contains(&Section);
}

bool UTrack_Command::IsEmpty() const
{
    return (Sections.Num() == 0);
}

void UTrack_Command::RemoveSection(UMovieSceneSection& Section)
{
    Sections.Remove(&Section);
}

FText UTrack_Command::GetDefaultDisplayName() const
{
    return LOCTEXT("UTrack_CommandName", "Command Track");
}

#undef LOCTEXT_NAMESPACE


