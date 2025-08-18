// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/SharedPointer.h"
#include "entity/fwd.hpp"
#include "ACTWorldSubsystem.generated.h"

/**
 * 
 */

class ACTGameEcsScene;
class UACTGlobalFrameManagerSystem;
class ACTGameEcsEntity;

UCLASS()
class ACTGAME_API UACTWorldSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    UACTWorldSubsystem();
    ~UACTWorldSubsystem();

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void BeginDestroy() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;


public:

private:
    UACTGlobalFrameManagerSystem* FrameManagerSystem;
};
