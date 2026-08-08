// Fill out your copyright notice in the Description page of Project Settings.


#include "Section_MoveInputAcceptance.h"

USection_MoveInputAcceptance::USection_MoveInputAcceptance(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

USection_MoveInputAcceptance::~USection_MoveInputAcceptance()
{
}

#if WITH_EDITOR
void USection_MoveInputAcceptance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


