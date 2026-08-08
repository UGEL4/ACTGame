// Fill out your copyright notice in the Description page of Project Settings.


#include "Track_MoveInputAcceptance.h"
#include "../Section/MoveInputAcceptance/Section_MoveInputAcceptance.h"

#define LOCTEXT_NAMESPACE "UTrack_MoveInputAcceptance"

UTrack_MoveInputAcceptance::UTrack_MoveInputAcceptance(const FObjectInitializer& InInitializer)
    : Super(InInitializer)
{
    SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
#if WITH_EDITORONLY_DATA
     TrackTint = FColor(25, 190, 9, 255);
#endif
 }

FName UTrack_MoveInputAcceptance::GetTrackName() const
 {
    static FName TestTrackName = TEXT("MoveInputAcceptanceTrack");
    return TestTrackName;
}


void UTrack_MoveInputAcceptance::AddSection(UMovieSceneSection& Section)
{
    Sections.Add(&Section);
}


bool UTrack_MoveInputAcceptance::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
    return SectionClass == USection_MoveInputAcceptance::StaticClass();
}


UMovieSceneSection* UTrack_MoveInputAcceptance::CreateNewSection()
{
    return NewObject<USection_MoveInputAcceptance>(this, NAME_None, RF_Transactional);
}

USection_MoveInputAcceptance* UTrack_MoveInputAcceptance::CreateNewMoveInputAcceptanceSection()
{
    return NewObject<USection_MoveInputAcceptance>(this, NAME_None, RF_Transactional);
}

const TArray<UMovieSceneSection*>& UTrack_MoveInputAcceptance::GetAllSections() const
{
    return Sections;
}

bool UTrack_MoveInputAcceptance::HasSection(const UMovieSceneSection& Section) const
{
    return Sections.Contains(&Section);
}

bool UTrack_MoveInputAcceptance::IsEmpty() const
{
    return (Sections.Num() == 0);
}

void UTrack_MoveInputAcceptance::RemoveSection(UMovieSceneSection& Section)
{
    Sections.Remove(&Section);
}

FText UTrack_MoveInputAcceptance::GetDefaultDisplayName() const
{
    return LOCTEXT("UTrack_MoveInputAcceptanceName", "MoveInputAcceptance Track");
}

#undef LOCTEXT_NAMESPACE


