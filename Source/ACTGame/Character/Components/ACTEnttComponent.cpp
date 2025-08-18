#include "ACTEnttComponent.h"
#include "Engine/World.h"
#include "Framework/ECS/Component/ACTGameTransformComponent.h"
#include "World/ACTWorldSubsystem.h"

UACTEnttComponent::UACTEnttComponent()
{

}

UACTEnttComponent::~UACTEnttComponent()
{

}

void UACTEnttComponent::BeginPlay()
{
    // UWorld* world                = GetWorld();
    // UACTWorldSubsystem* ACTWorld = world->GetSubsystem<UACTWorldSubsystem>();
    // Entity = ACTWorld->CreateEntity();
    // Entity.AddComponent<ACTGame::PositionComponent>(0.f, 0.f, 0.f);
    // FVector location = GetOwner()->GetActorLocation();
}