// Fill out your copyright notice in the Description page of Project Settings.


#include "Section_Command.h"

USection_Command::USection_Command(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

USection_Command::~USection_Command()
{
}

#if WITH_EDITOR
void USection_Command::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif


