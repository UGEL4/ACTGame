// Fill out your copyright notice in the Description page of Project Settings.


#include "Section_HitBox.h"
#include "ActionEditor/Actor/HitBox/HitBoxActor.h"

USection_HitBox::USection_HitBox(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

USection_HitBox::~USection_HitBox()
{
    UE_LOG(LogTemp, Log, TEXT("USection_HitBox Destruct"));
}

int32 USection_HitBox::GetKeyframeIndexAtTime(FFrameNumber InFrame) const
{
    int32 Index = Algo::BinarySearchBy(Keyframes, InFrame, [](const FCapsuleKeyframeData& Data) { return Data.Time; });
    return Index < Keyframes.Num() ? Index : INDEX_NONE;
}

void USection_HitBox::AddKeyFrame(FFrameNumber InFrame, bool Sort)
{
    int32 Index = GetKeyframeIndexAtTime(InFrame);
    if (Index >= 0)
    {

    }
    else
    {
        FCapsuleKeyframeData Data;
        Data.Time = InFrame;
        Keyframes.Add(Data);
        if (Sort)
        {
            Keyframes.Sort([this](const FCapsuleKeyframeData& a, const FCapsuleKeyframeData& b)
                           { return a.Time.Value < b.Time.Value; });
        }
    }
}

bool USection_HitBox::RemoveKeyFrame(FFrameNumber InFrame)
{
    int32 Index = GetKeyframeIndexAtTime(InFrame);
    if (Index >= 0)
    {
        RemoveActorsForFrame(InFrame);
        Keyframes.RemoveAt(Index);
        return true;
    }
    return false;
}

void USection_HitBox::SetActorsForFrame(FFrameNumber Frame, TArray<TWeakObjectPtr<AHitBoxActor>> Actors)
{
    auto Info = KeyframeToActor.Find(Frame);
    if (Info)
    {
        Info->Actors.Append(Actors);
    }
    else
    {
        FFrameHitBoxActor NewInfo;
        NewInfo.Actors = Actors;
        KeyframeToActor.Add(Frame, NewInfo);
    }
}

void USection_HitBox::SetActorForFrame(FFrameNumber Frame, AHitBoxActor* Actor)
{
    if (Actor)
    {
        auto Info = KeyframeToActor.Find(Frame);
        if (Info)
        {
            if (!Info->Actors.Contains(Actor))
            {
                Info->Actors.Add(Actor);
            }
        }
        else
        {
            FFrameHitBoxActor NewInfo;
            NewInfo.Actors.Add(Actor);
            KeyframeToActor.Add(Frame, NewInfo);
        }
    }
}

void USection_HitBox::GetActorsForFrame(FFrameNumber Frame, TArray<TWeakObjectPtr<AHitBoxActor>>& OutActors) const
{
    if (auto Found = KeyframeToActor.Find(Frame))
    {
        OutActors = Found->Actors;
    }
}

bool USection_HitBox::HasAnyHitBoxInFrame(FFrameNumber Frame) const
{
    if (auto Found = KeyframeToActor.Find(Frame))
    {
        for (auto Actor : Found->Actors)
        {
            auto Ptr = Actor.Pin();
            if (Ptr.IsValid())
            {
                return true;
            }
        }
    }
    return false;
}

void USection_HitBox::RemoveActorForFrame(FFrameNumber Frame, AHitBoxActor* Actor)
{
    if (auto Info = KeyframeToActor.Find(Frame))
    {
        for (int32 i = 0; i < Info->Actors.Num(); i++)
        {
            if (Actor == Info->Actors[i].Get())
            {
                Info->Actors.RemoveAt(i);
                break;
            }
        }
    }
}

bool USection_HitBox::RemoveActorsForFrame(FFrameNumber Frame)
{
    bool HasChange = false;
    if (auto Info = KeyframeToActor.Find(Frame))
    {
        for (int32 i = 0; i < Info->Actors.Num(); i++)
        {
            if (auto Actor = Info->Actors[i].Pin())
            {
                if (Actor.IsValid())
                {
                    Actor->Destroy();
                }
                HasChange = true;
            }
        }
        Info->Actors.Empty();
    }
    return HasChange;
}

void USection_HitBox::ClearAllActors()
{
    for (auto& Pair : KeyframeToActor)
    {
        for (auto Actor : Pair.Value.Actors)
        {
            if (auto Ptr = Actor.Get())
            {
                Ptr->Destroy();
            }
        }
    }
    KeyframeToActor.Empty();
}

bool USection_HitBox::ClearInvalidHitBoxActor()
{
    bool HasChange = false;
    for (auto& Pair : KeyframeToActor)
    {
        for (int32 i = Pair.Value.Actors.Num() - 1; i >= 0; --i)
        {
            auto ActorPtr = Pair.Value.Actors[i].Pin();
            if (!ActorPtr.IsValid())
            {
                Pair.Value.Actors.RemoveAt(i);
                HasChange = true;
            }
        }
    }
    return HasChange;
}

void USection_HitBox::BeginDestroy()
{
    ClearAllActors();
    Super::BeginDestroy();
}

#if WITH_EDITOR
void USection_HitBox::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


