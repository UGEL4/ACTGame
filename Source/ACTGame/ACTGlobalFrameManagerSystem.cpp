// Fill out your copyright notice in the Description page of Project Settings.


#include "ACTGlobalFrameManagerSystem.h"
#include "CoreGlobals.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Event/ACTGameEvent.h"
#include "Framework/Event/ACTGameEventParam.h"
#include "Logging/LogMacros.h"
#include "Logging/LogVerbosity.h"

bool UACTGlobalFrameManagerSystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

/** Implement this for initialization of instances of the system */
void UACTGlobalFrameManagerSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Ticker.bCanEverTick = true;
	Ticker.bStartWithTickEnabled = true;
	Ticker.bAllowTickBatching = true;
	Ticker.bRunTransactionally = true;
	RegisterTickFunction(nullptr);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UACTGlobalFrameManagerSystem::PostLoadMap);

	ACTGame::EventService EventService;
	EventService.RegisterHandler<ACTGame::EntityCreatedParam>((int)ACTGame::EventType::EntityCreated, [](const ACTGame::EntityCreatedParam& param)
	{
		UE_LOG(LogTemp, Warning, TEXT("创建了实体: entity:%lld scene:%lld"), (size_t)param.Entity, (size_t)param.Scene);
	});
}

/** Implement this for deinitialization of instances of the system */
void UACTGlobalFrameManagerSystem::Deinitialize()
{
	Super::Deinitialize();
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	UnRegisterTickFunction();
}

void UACTGlobalFrameManagerSystem::Tick(float DeltaTime)
{
	AccumulateTime += DeltaTime;
	NowTime += DeltaTime;
	while (AccumulateTime >= FixedDeltaTime)
	{
		AccumulateTime -= FixedDeltaTime;
		CurrentFrame += 1;
	}
}

void UACTGlobalFrameManagerSystem::RegisterTickFunction(UWorld* world)
{
	Ticker.Target = this;
	Ticker.SetTickFunctionEnable(Ticker.bStartWithTickEnabled || Ticker.IsTickFunctionEnabled());
	if (world == nullptr)
	{
		Ticker.RegisterTickFunction(GetGameInstance()->GetWorld()->PersistentLevel);
	}
	else
	{
		Ticker.RegisterTickFunction(world->PersistentLevel);
	}
}

void UACTGlobalFrameManagerSystem::UnRegisterTickFunction()
{
	if (Ticker.IsTickFunctionRegistered())
	{
		Ticker.UnRegisterTickFunction();
	}
}

void UACTGlobalFrameManagerSystem::PostLoadMap(UWorld* world)
{
	if (world != nullptr)
	{
		RegisterTickFunction(world);
	}
}

