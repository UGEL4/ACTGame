// Fill out your copyright notice in the Description page of Project Settings.


#include "Track_HitBox.h"
#include "../Section/HitBox/Section_HitBox.h"

#define LOCTEXT_NAMESPACE "UTrack_HitBox"

UTrack_HitBox::UTrack_HitBox(const FObjectInitializer& InInitializer)
    : Super(InInitializer)
{
    SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
#if WITH_EDITORONLY_DATA
     TrackTint = FColor(200, 190, 9, 255);
#endif
 }

FName UTrack_HitBox::GetTrackName() const
 {
    static FName TestTrackName = TEXT("HitBoxTrack");
    return TestTrackName;
}


void UTrack_HitBox::AddSection(UMovieSceneSection& Section)
{
    Sections.Add(&Section);
}


bool UTrack_HitBox::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
    return SectionClass == USection_HitBox::StaticClass();
}


UMovieSceneSection* UTrack_HitBox::CreateNewSection()
{
    return NewObject<USection_HitBox>(this, NAME_None, RF_Transactional);
}

const TArray<UMovieSceneSection*>& UTrack_HitBox::GetAllSections() const
{
    return Sections;
}

bool UTrack_HitBox::HasSection(const UMovieSceneSection& Section) const
{
    return Sections.Contains(&Section);
}

bool UTrack_HitBox::IsEmpty() const
{
    return (Sections.Num() == 0);
}

void UTrack_HitBox::RemoveSection(UMovieSceneSection& Section)
{
    if (USection_HitBox* HitBoxSection = Cast<USection_HitBox>(&Section))
    {
        HitBoxSection->ClearAllActors();
    }
    Sections.Remove(&Section);
}

FText UTrack_HitBox::GetDefaultDisplayName() const
{
    return LOCTEXT("UTrack_HitBoxName", "HitBox Track");
}

bool UTrack_HitBox::ClearInvalidHitBoxActor()
{
    bool HasChange = false;
    for (auto Section : Sections)
    {
        if (Section)
        {
            if (Cast<USection_HitBox>(Section)->ClearInvalidHitBoxActor())
            {
                HasChange = true;
            }
        }
    }
    return HasChange;
}

#undef LOCTEXT_NAMESPACE


