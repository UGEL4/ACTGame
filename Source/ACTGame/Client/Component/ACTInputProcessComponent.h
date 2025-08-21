#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ACTInputProcessComponent.generated.h"

UCLASS()
class ACTGAME_API UACTInputProcessComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UACTInputProcessComponent();
    ~UACTInputProcessComponent();
    
    virtual void BeginPlay() override;
    
    void OnLeftStickInput(float x, float y);
};

