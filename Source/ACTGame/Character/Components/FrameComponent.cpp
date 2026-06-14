#include "FrameComponent.h"

UFrameComponent::UFrameComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UFrameComponent::BeginPlay()
{
    Super::BeginPlay();

    AccumulateTime = 0.0;
	CurrentLogicFrame = 0;
}

void UFrameComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AccumulateTime += DeltaTime;
	while (AccumulateTime >= FixedDeltaTime)
	{
		AccumulateTime -= FixedDeltaTime;
		CurrentLogicFrame++;
	}
}