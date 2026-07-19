// Fill out your copyright notice in the Description page of Project Settings.


#include "Section_CancelTag.h"

USection_CancelTag::USection_CancelTag(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

USection_CancelTag::~USection_CancelTag()
{
}

#if WITH_EDITOR
void USection_CancelTag::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


