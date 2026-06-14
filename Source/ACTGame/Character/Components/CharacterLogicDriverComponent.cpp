// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterLogicDriverComponent.h"
#include "InputToCommandComponent.h"
#include "ActionLogicComponent.h"
#include "ACTGameCharacter.h"


// Sets default values for this component's properties
UCharacterLogicDriverComponent::UCharacterLogicDriverComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCharacterLogicDriverComponent::BeginPlay()
{
    Super::BeginPlay();

    // ...
    AccumulateTime    = 0.0;
    CurrentLogicFrame = 0;

    auto ch                 = Cast<AACTGameCharacter>(GetOwner());
    InputToCommandComponent = ch->GetInputToCommandComponent();
    ActionLogicComponent    = ch->GetActionLogicComponent();
}

// Called every frame
void UCharacterLogicDriverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AccumulateTime += DeltaTime;
	while (AccumulateTime >= FixedDeltaTime)
	{
		AccumulateTime -= FixedDeltaTime;
		CurrentLogicFrame++;

		InputToCommandComponent->TickLogic(CurrentLogicFrame);
		ActionLogicComponent->TickLogic(CurrentLogicFrame);
	}
}

