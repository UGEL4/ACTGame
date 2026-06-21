// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterLogicDriverComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ACTGAME_API UCharacterLogicDriverComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterLogicDriverComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	double AccumulateTime = 0.0;
	double FixedDeltaTime = 1 / 60.0;
	int64 CurrentLogicFrame = 0;

private:
	class UInputToCommandComponent* InputToCommandComponent;
	class UActionLogicComponent* ActionLogicComponent;
    class UCharacterMovementComponent* MovementComponent;
	
};
