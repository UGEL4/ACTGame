// Fill out your copyright notice in the Description page of Project Settings.


#include "Track_ActionInfo.h"
#include "../Section/Section_ActionInfo.h"

#define LOCTEXT_NAMESPACE "UTrack_ActionInfo"

 UTrack_ActionInfo::UTrack_ActionInfo(const FObjectInitializer& InInitializer)
    : Super(InInitializer)
{
    SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
#if WITH_EDITORONLY_DATA
     TrackTint = FColor(190, 50, 9, 255);
#endif
 }

FName UTrack_ActionInfo::GetTrackName() const
 {
    static FName TestTrackName = TEXT("CancelTagTrack");
    return TestTrackName;
}


void UTrack_ActionInfo::AddSection(UMovieSceneSection& Section)
{
    Sections.Add(&Section);
}


bool UTrack_ActionInfo::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
    return SectionClass == USection_ActionInfo::StaticClass();
}


UMovieSceneSection* UTrack_ActionInfo::CreateNewSection()
{
    return NewObject<USection_ActionInfo>(this, NAME_None, RF_Transactional);
}

const TArray<UMovieSceneSection*>& UTrack_ActionInfo::GetAllSections() const
{
    return Sections;
}

bool UTrack_ActionInfo::HasSection(const UMovieSceneSection& Section) const
{
    return Sections.Contains(&Section);
}

bool UTrack_ActionInfo::IsEmpty() const
{
    return (Sections.Num() == 0);
}

void UTrack_ActionInfo::RemoveSection(UMovieSceneSection& Section)
{
    Sections.Remove(&Section);
}

FText UTrack_ActionInfo::GetDefaultDisplayName() const
{
    return LOCTEXT("UTrack_ActionInfoName", "ActionInfo Track");
}

#undef LOCTEXT_NAMESPACE


