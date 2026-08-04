// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HitBoxActor.generated.h"

class UCapsuleComponent;

UCLASS()
class ACTGAME_API AHitBoxActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHitBoxActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    FVector GetCapsuleParam() const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCapsuleComponent> HitBox;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    int32 Frame;

    UPROPERTY(EditAnywhere)
    float Attack = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    FName SameTag;
};
