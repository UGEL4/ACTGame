// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HAL/Platform.h"
#include "UObject/ObjectMacros.h"
#include "FrameComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ACTGAME_API UFrameComponent : public UActorComponent
{
    GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFrameComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    int64 GetCurrentLogicFrame() const { return CurrentLogicFrame; }
private:
	double AccumulateTime = 0.0;
	double FixedDeltaTime = 1 / 60.0;
	int64 CurrentLogicFrame = 0;
};