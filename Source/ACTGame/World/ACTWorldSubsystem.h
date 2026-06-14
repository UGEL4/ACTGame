// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/IDelegateInstance.h"
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
class IACTLogicCharacter;

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

    ACTGameEcsScene* GetGameEcsScene()
    {
        return EcsScene.Get();
    }

public:
    ACTGameEcsEntity* CreateEntity();

private:
    TSharedPtr<ACTGameEcsScene> EcsScene;
    UACTGlobalFrameManagerSystem* FrameManagerSystem;

private:
    void RunLogicFrame();
    void DealWithAttacks();
    void DealWithInputs();
    void DealWithActions();
    void DealWithMovements();

    double AccumulateTime = 0.0;
	double FixedDeltaTime = 1 / 60.0;
	int64 CurrentLogicFrame = 0;

public:
    // 注册/注销需要逻辑更新的角色
    void RegisterCharacter(IACTLogicCharacter* Character, bool IsPlayer);
    void UnregisterCharacter(IACTLogicCharacter* Character, bool IsPlayer);
private:
    IACTLogicCharacter* Player;
    TArray<IACTLogicCharacter*> Enemys;
};
