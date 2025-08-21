#include "ACTInputProcessComponent.h"
#include "ACTEnttComponent.h"
#include "Framework/ACTGameGameInstance.h"
#include "Framework/Game/Command/ActionCommand.h"
#include "GameFramework/Controller.h"
#include "ACTGameCharacter.h"
#include "Math/MathFwd.h"
#include "Framework/ECS/Component/Command/ACTCommandComponent.h"

UACTInputProcessComponent::UACTInputProcessComponent()
{
    bNeverNeedsRenderUpdate = false;
}

UACTInputProcessComponent::~UACTInputProcessComponent()
{

}

void UACTInputProcessComponent::BeginPlay()
{
    Super::BeginPlay();
    SetComponentTickEnabled(false);
}

void UACTInputProcessComponent::OnLeftStickInput(float x, float y)
{
    const AACTGameCharacter* OwnerActor = Cast<AACTGameCharacter>(GetOwner());
    if (!OwnerActor) { return; }
    UACTEnttComponent* EnttComponent = OwnerActor->GetEnttComponent();
    if (!EnttComponent) { return; }

    // const FRotator Rotation = OwnerActor->GetController()->GetControlRotation();
    // const FRotator YawRotation(0, Rotation.Yaw, 0);

    // // get forward vector
    // const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

    // // get right vector
    // const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    const FVector2D Input(x, y);
    if (Input.Length() > 0.01f)
    {

        ACTGame::CommandComponent* CommandComp = EnttComponent->GetEntity()->TryGetComponent<ACTGame::CommandComponent>();
        if (CommandComp)
        {
            ACTGame::KeyInputRecord InputRecord;
            ACTGame::ACTGameInstance& GameInstance = ACTGame::ACTGameInstance::Get();
            InputRecord.Frame                      = GameInstance.GetFrameManager().GetCurrentFrame();
            InputRecord.Key                        = ACTGame::KeyMap::DirInput;
            CommandComp->Commands.emplace_back(InputRecord);
        }
    }
}