// Fill out your copyright notice in the Description page of Project Settings.


#include "Track_CancelTag.h"
#include "../Section/Section_CancelTag.h"

#define LOCTEXT_NAMESPACE "UTrack_CancelTag"

 UTrack_CancelTag::UTrack_CancelTag(const FObjectInitializer& InInitializer)
    : Super(InInitializer)
{
    SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
#if WITH_EDITORONLY_DATA
     TrackTint = FColor(25, 190, 9, 255);
#endif
 }

FName UTrack_CancelTag::GetTrackName() const
{
    static FName TestTrackName = TEXT("CancelTagTrack");
    return TestTrackName;
}


void UTrack_CancelTag::AddSection(UMovieSceneSection& Section)
{
    Sections.Add(&Section);
}


bool UTrack_CancelTag::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
    return SectionClass == USection_CancelTag::StaticClass();
}


UMovieSceneSection* UTrack_CancelTag::CreateNewSection()
{
    return NewObject<USection_CancelTag>(this, NAME_None, RF_Transactional);
}

const TArray<UMovieSceneSection*>& UTrack_CancelTag::GetAllSections() const
{
    return Sections;
}

bool UTrack_CancelTag::HasSection(const UMovieSceneSection& Section) const
{
    return Sections.Contains(&Section);
}

bool UTrack_CancelTag::IsEmpty() const
{
    return (Sections.Num() == 0);
}

void UTrack_CancelTag::RemoveSection(UMovieSceneSection& Section)
{
    Sections.Remove(&Section);
}

FText UTrack_CancelTag::GetDefaultDisplayName() const
{
    return LOCTEXT("UTrack_CancelTagName", "CancelTag Track");
}

#undef LOCTEXT_NAMESPACE


