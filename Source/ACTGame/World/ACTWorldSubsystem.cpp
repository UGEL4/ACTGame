// Fill out your copyright notice in the Description page of Project Settings.


#include "World/ACTWorldSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/ECS/Component/ACTGameTransformComponent.h"
#include "Framework/ECS/Scene/ACTGameEcsScene.h"
#include "Framework/ECS/Entity/ACTGameEcsEntity.h"
#include "Framework/Event/ACTGameEvent.h"
#include "Framework/Event/ACTGameEventParam.h"

UACTWorldSubsystem::UACTWorldSubsystem()
{
}

UACTWorldSubsystem::~UACTWorldSubsystem()
{
    
}

bool UACTWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    UWorld* World = Cast<UWorld>(Outer);
	check(World);
    return World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE;
}

void UACTWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //EcsScene->Initialize();
    //ACTGameEcsEntity* DefaultEntity = CreateEntity();
    //DefaultEntity->AddComponent<ACTGame::PositionComponent>(0.f, 0.f, 0.f);
    //DefaultEntity->AddComponent<ACTGame::RotationComponent>(0.f, 0.f, 0.f, 1.f);
    //DefaultEntity->AddComponent<ACTGame::ScaleComponent>(1.f, 1.f, 1.f);
    //ACTGame::EventService EventService;
    //EventService.Emit((int)ACTGame::EventType::EntityCreated, ACTGame::EntityCreatedParam(DefaultEntity, EcsScene.Get()));
}

void UACTWorldSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UACTWorldSubsystem::BeginDestroy()
{
    Super::BeginDestroy();
}

void UACTWorldSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

TStatId UACTWorldSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UClassName, STATGROUP_Tickables);
}

