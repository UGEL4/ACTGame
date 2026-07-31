// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionEditor/Actor/HitBox/HitBoxActor.h"
#include "Components/CapsuleComponent.h"


// Sets default values
AHitBoxActor::AHitBoxActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	HitBox = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitBox"));
    HitBox->InitCapsuleSize(34.0f, 88.0f);
    HitBox->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
    HitBox->SetHiddenInGame(false);

    HitBox->SetShouldUpdatePhysicsVolume(false);
    HitBox->SetCanEverAffectNavigation(false);
    RootComponent = HitBox;

}

// Called when the game starts or when spawned
void AHitBoxActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHitBoxActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

