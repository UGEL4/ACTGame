// Fill out your copyright notice in the Description page of Project Settings.


#include "World/ACTWorldSubsystem.h"
#include "ACTGlobalFrameManagerSystem.h"
#include "Containers/Ticker.h"
#include "Engine/HitResult.h"
#include "Engine/ScopedMovementUpdate.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Framework/ECS/Component/ACTGameTransformComponent.h"
#include "Framework/ECS/Scene/ACTGameEcsScene.h"
#include "Framework/ECS/Entity/ACTGameEcsEntity.h"
#include "Framework/Event/ACTGameEvent.h"
#include "Framework/Event/ACTGameEventParam.h"
#include "Character/IACTLogicCharacter.h"
#include "Character/Components/InputToCommandComponent.h"
#include "Character/Components/ActionLogicComponent.h"
#include "Framework/Game/Action/ActionRootMotionSource.h"
#include "Math/MathFwd.h"

UACTWorldSubsystem::UACTWorldSubsystem()
{
    //EcsScene = MakeShared<ACTGameEcsScene>();
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

    //FrameManagerSystem = GetWorld()->GetGameInstance()->GetSubsystem<UACTGlobalFrameManagerSystem>();
    //EcsScene->Initialize();
    //ACTGameEcsEntity* DefaultEntity = CreateEntity();
    //DefaultEntity->AddComponent<ACTGame::PositionComponent>(0.f, 0.f, 0.f);
    //DefaultEntity->AddComponent<ACTGame::RotationComponent>(0.f, 0.f, 0.f, 1.f);
    //DefaultEntity->AddComponent<ACTGame::ScaleComponent>(1.f, 1.f, 1.f);
    //ACTGame::EventService EventService;
    //EventService.Emit((int)ACTGame::EventType::EntityCreated, ACTGame::EntityCreatedParam(DefaultEntity, EcsScene.Get()));

    AccumulateTime    = 0.0;
	CurrentLogicFrame = 0;
}

void UACTWorldSubsystem::Deinitialize()
{
    Super::Deinitialize();
    //EcsScene->Deinitialize();
    //FrameManagerSystem = nullptr;
}

void UACTWorldSubsystem::BeginDestroy()
{
    Super::BeginDestroy();
}

void UACTWorldSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // if (EcsScene != nullptr)
    // {
    //     EcsScene->Update(DeltaTime, FrameManagerSystem->GetCurrentFrame());
    // }

    AccumulateTime += DeltaTime;
    while (AccumulateTime >= FixedDeltaTime)
    {
        AccumulateTime -= FixedDeltaTime;
        CurrentLogicFrame++;
        //RunLogicFrame();
    }
}

TStatId UACTWorldSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UClassName, STATGROUP_Tickables);
}

ACTGameEcsEntity* UACTWorldSubsystem::CreateEntity()
{
    return EcsScene->CreateEntity();
}

void UACTWorldSubsystem::RunLogicFrame()
{
    // 处理攻击
    DealWithAttacks();
    // 输入
    DealWithInputs();
    // 动作
    DealWithActions();
    // 移动
    DealWithMovements();
}

void UACTWorldSubsystem::DealWithAttacks()
{
    for (auto Enemy : Enemys)
    {
        // 玩家对敌人
        if (Player && Enemy)
        {

        }

        // 敌人对玩家
        if (Player && Enemy)
        {

        }
    }
}

void UACTWorldSubsystem::DealWithInputs()
{
    if (Player)
    {
        if (auto Input = Player->GetInputToCommandComponent())
        {
            Input->TickLogic(CurrentLogicFrame);
        }
    }
    for (auto Enemy : Enemys)
    {
        if (!Enemy)
        {
            continue;
        }
        if (auto Input = Enemy->GetInputToCommandComponent())
        {
            Input->TickLogic(CurrentLogicFrame);
        }
    }
}

void UACTWorldSubsystem::DealWithActions()
{
    if (Player)
    {
        if (auto Action = Player->GetActionLogicComponent())
        {
            Action->TickLogic(CurrentLogicFrame);
        }
    }
    for (auto Enemy : Enemys)
    {
        if (!Enemy)
        {
            continue;
        }
        if (auto Action = Enemy->GetActionLogicComponent())
        {
            Action->TickLogic(CurrentLogicFrame);
        }
    }
}

void UACTWorldSubsystem::DealWithMovements()
{
    if (Player)
    {
        // auto Owner = Cast<AACTGameCharacter>(Player);
        // FLogicMovementIntent Intent = Player->GetMovementIntent();
        // if (Intent.bHasMovement)
        // {
        //     auto CMC = Player->GetCharacterMovementComponent();
        //     if (CMC)
        //     {
        //         FScopedMovementUpdate ScopedMovementUpdate(Owner->GetRootComponent(), EScopedUpdate::DeferredUpdates);
        //         FHitResult Hit;
        //         //CMC->MoveUpdatedComponent(const FVector &Delta, const FQuat &NewRotation, bool bSweep)
        //         //CMC->MoveUpdatedComponent(const FVector &Delta, const FRotator &NewRotation, bool bSweep)
        //     }
        // }

        // {
        //     // 1. 创建自定义源
        //     FActionRootMotionSource* NewSource = new FActionRootMotionSource();
        //     //NewSource->SequenceData = Player->GetActionLogicComponent(); // 你的数据资产
        //     //NewSource->CurrentTime = 0.0f;
        //     NewSource->Duration = PreloadedSequenceAsset->GetTotalDuration();
        //     NewSource->AccumulateMode = ERootMotionAccumulateMode::Override; // 或其他模式
        //     NewSource->Priority = 100; // 确保覆盖其他源
        //     const FTransform& RootMotion = Player->GetActionLogicComponent()->GetRootMotionTransform();
        //     NewSource->RootMotionParams.Set(RootMotion);

        //     // 2. 应用到CMC
        //     UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
        //     CMC->ApplyRootMotionSource(NewSource);
        // }

        // 3. 手动推进动画（如果需要IK或事件等，这里可按需跳过，因为我们不再依赖动画蓝图）
        // 但你需要确保CMC的Tick被调用，以便它处理根运动源。
        // 按照之前的设计：
        // if (UCharacterMovementComponent* CMC = Cast<AACTGameCharacter>(Player)->GetCharacterMovement())
        // {
        //     CMC->AddInputVector(FVector(1, 0, 0));
        //     CMC->Velocity = FVector(10, 0, 0);
        //     // 注意：这里调用TickComponent会触发根运动源的PrepareRootMotion
        //     CMC->TickComponent(FixedDeltaTime, ELevelTick::LEVELTICK_All, nullptr);
        //     //CMC->PerformMovement(FixedDeltaTime);
        // }
    }

    for (auto Enemy : Enemys)
    {
        if (!Enemy)
        {
            continue;
        }
    }
}


void UACTWorldSubsystem::RegisterCharacter(IACTLogicCharacter* Character, bool IsPlayer)
{
    if (IsPlayer)
    {
        Player = Character;
    }
    else
    {
        bool Added = Enemys.ContainsByPredicate([Character](const auto element) {
            return element->GetObjectId() == Character->GetObjectId();
        });
        if (Added)
        {
            return;
        }
        Enemys.Add(Character);
    }
}

void UACTWorldSubsystem::UnregisterCharacter(IACTLogicCharacter* Character, bool IsPlayer)
{
    if (IsPlayer)
    {
        Player = nullptr;
    }
    else
    {
        Enemys.RemoveAll([Character](const auto element) {
            return element->GetObjectId() == Character->GetObjectId();
        });
    }
}
