#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Framework/ECS/Entity/ACTGameEcsEntity.h"
#include "ACTEnttComponent.generated.h"

UCLASS()
class ACTGAME_API UACTEnttComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UACTEnttComponent();
    ~UACTEnttComponent();
    
    virtual void BeginPlay() override;

private:
    ACTGameEcsEntity* Entity;
};

