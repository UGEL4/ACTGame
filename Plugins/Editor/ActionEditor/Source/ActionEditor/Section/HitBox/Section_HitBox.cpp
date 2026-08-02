// Fill out your copyright notice in the Description page of Project Settings.


#include "Section_HitBox.h"
#include "ActionEditor/Actor/HitBox/HitBoxActor.h"

USection_HitBox::USection_HitBox(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

USection_HitBox::~USection_HitBox()
{
}

int32 USection_HitBox::GetKeyframeIndexAtTime(FFrameNumber InFrame, float Tolerance /*= 0.1f*/) const
{
    //// 将容差转换为帧数（假设30fps，可根据实际Tick分辨率调整）
    //FFrameNumber ToleranceFrames = FFrameNumber(FMath::CeilToInt(Tolerance * 30.0f));

    //for (int32 i = 0; i < Keyframes.Num(); ++i)
    //{
    //    if (FMath::Abs(Keyframes[i].Time.Value - InTime.FrameNumber.Value) <= ToleranceFrames.Value)
    //    {
    //        return i;
    //    }
    //}
    int32 Index = Algo::BinarySearchBy(Keyframes, InFrame, [](const FCapsuleKeyframeData& Data) { return Data.Time; });
    return Index < Keyframes.Num() ? Index : INDEX_NONE;
}

void USection_HitBox::AddKeyFrame(FFrameNumber InFrame)
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
        Keyframes.Sort([this](const FCapsuleKeyframeData& a, const FCapsuleKeyframeData& b)
                       { return a.Time.Value < b.Time.Value; });
    }
}

void USection_HitBox::SetActorForFrame(FFrameNumber Frame, AHitBoxActor* Actor)
{
    if (Actor)
    {
        KeyframeToActor.Add(Frame, Actor);
    }
}

AHitBoxActor* USection_HitBox::GetActorForFrame(FFrameNumber Frame) const
{
    if (auto Found = KeyframeToActor.Find(Frame))
    {
        return Found->Get();
    }
    return nullptr;
}

void USection_HitBox::RemoveActorForFrame(FFrameNumber Frame)
{
    if (auto Actor = GetActorForFrame(Frame))
    {
        Actor->Destroy();
    }
    KeyframeToActor.Remove(Frame);
}

void USection_HitBox::ClearAllActors()
{
    for (auto& Pair : KeyframeToActor)
    {
        if (auto Actor = Pair.Value.Get())
        {
            Actor->Destroy();
        }
    }
    KeyframeToActor.Empty();
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


