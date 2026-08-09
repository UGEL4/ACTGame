#include "Character/Components/InputToCommandComponent.h"

#include "ACTGameCharacter.h"
#include "FrameComponent.h"
#include "Framework/Game/Command/ActionCommand.h"
#include "HAL/Platform.h"
#include "InputToCommandComponent.h"

UInputToCommandComponent::UInputToCommandComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInputToCommandComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentLogicFrame = 0;
    NonDirectCooldown = 0;
	PendingCooldown = false;
    NoDirInput      = true;
    Inputs.Empty();
}

void UInputToCommandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInputToCommandComponent::TickLogic(int64 CurrentFrame)
{
    CurrentLogicFrame = CurrentFrame;
    UpdateCommand();
    if (PendingCooldown)
    {
        NonDirectCooldown = (int32)CurrentFrame + NonDirectionInputCooldown;
        PendingCooldown   = false;
    }
}

void UInputToCommandComponent::TryAddInput(EKeyMap Key)
{
    if (Key != EKeyMap::NoInput && Key != EKeyMap::DirInput)
    {
        if (CurrentLogicFrame < NonDirectCooldown)
        {
            return;
        }
        Inputs.Emplace(CurrentLogicFrame, Key);
        PendingCooldown = true;
    }
}

void UInputToCommandComponent::TryAddDirectInput(FVector2D Input)
{
    if (Input.SizeSquared() > 0.0001f)
    {
        Inputs.Emplace(CurrentLogicFrame, EKeyMap::DirInput, Input);
        NoDirInput = false;
    }
    else
    {
        NoDirInput = true;
    }
}

bool UInputToCommandComponent::ActionOccur(const FActionCommand& Command)
{
    int32 LastFrame = CurrentLogicFrame - FMath::Max(Command.ValidInFrameCount, 1); // 最小就是1帧了
    for (int32 i = 0; i < Command.keySequences.Num(); i++)
    {
        int32 idx = 0;
        bool found = false;
        for (int32 j = 0; j < Inputs.Num(); j++)
        {
            // if (Command.keySequences[i] == Inputs[j].Key && (CurrentLogicFrame - Inputs[j].FrameIndex) <= Command.ValidInFrameCount)
            if (Inputs[j].FrameIndex >= LastFrame && Command.keySequences[i] == Inputs[j].Key)
            {
                idx = j;
                found = true;
                break;
            }
        }
        if (found)
        {
            continue;
        }

        return false;
    }
    return true;
}

bool UInputToCommandComponent::ActionOccur(const FActionCommand& Command, FKeyInputRecord& OutData)
{
    int32 LastFrame = CurrentLogicFrame - FMath::Max(Command.ValidInFrameCount, 1); // 最小就是1帧了
    for (int32 i = 0; i < Command.keySequences.Num(); i++)
    {
        int32 idx  = 0;
        bool found = false;
        for (int32 j = 0; j < Inputs.Num(); j++)
        {
            if (Inputs[j].FrameIndex >= LastFrame && Command.keySequences[i] == Inputs[j].Key)
            {
                idx   = j;
                found = true;
                OutData = Inputs[i];
                break;
            }
        }
        if (found)
        {
            continue;
        }

        return false;
    }
    return true;
}

void UInputToCommandComponent::UpdateCommand()
{
    int32 index = 0;
    while (index < Inputs.Num())
    {
        if (CurrentLogicFrame - Inputs[index].FrameIndex > RecordKeepFrame)
        {
            Inputs.RemoveAt(index);
        }
        else
        {
            index++;
        }
    }

    // 处理输入
    //if (!NoKeyInput && )

    if (NoDirInput)
    {
        Inputs.Emplace(CurrentLogicFrame, EKeyMap::NoDir);
    }
}
