// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Components/ActionLogicComponent.h"
#include "Character/Components/InputToCommandComponent.h"
#include "Engine/Engine.h"
#include "Framework/Game/Action/ActionInfo.h"
#include "Framework/Game/Action/CancelTag.h"
#include "Framework/Game/Action/PreorderActionInfo.h"
#include "ACTGameCharacter.h"

// Sets default values for this component's properties
UActionLogicComponent::UActionLogicComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = false;

    // ...
}

// Called when the game starts
void UActionLogicComponent::BeginPlay()
{
    Super::BeginPlay();

    // ...
    CurrentLogicFrame = 0;
    CurrentActionFrameIndex = 0;
    RootMotionMove = FVector::ZeroVector;
}

// Called every frame
void UActionLogicComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // ...
}

void UActionLogicComponent::TickLogic(int64 CurrentFrame)
{
    CurrentLogicFrame = CurrentFrame;
    UpdateAction();
}

void UActionLogicComponent::UpdateAction()
{
    if (CurrentAction == nullptr)
    {
        return;
    }

    int32 LastActionFrameIndex = CurrentActionFrameIndex;
    CurrentActionFrameIndex += 1;
    if (CurrentActionFrameIndex >= CurrentAction->FrameNum)
    {
        // todo: 待修正
        CurrentActionFrameIndex = 0;
    }
    for (const auto& Action : ActionList)
    {
        FBeCancelledTag BeCancelledTag;
        FCancelTag CancelTag;
        if (CanCancelCurrent(Action, CurrentActionFrameIndex, true, BeCancelledTag, CancelTag))
        {
            // 预约动作
            PreorderActionList.Emplace(Action.Name, Action.Priority + BeCancelledTag.Priority + CancelTag.Priority);
        }
    }
    if (PreorderActionList.Num() <= 0 && (CurrentActionFrameIndex + 1 >= CurrentAction->FrameNum || CurrentAction->AutoTerminate))
    {
        PreorderActionList.Emplace(CurrentAction->AutoNextActionId, 0);
    }
    if (PreorderActionList.Num() > 0)
    {
        PreorderActionList.Sort([](const auto& a, const auto& b)
                                { return a.Priority > b.Priority; });
        if (PreorderActionList[0].ActionId == CurrentAction->Name && CurrentAction->KeepPlayingAnimation)
        {
            KeepAction();
        }
        else
        {
            ChangeAction(PreorderActionList[0].ActionId);
        }
    }
    PreorderActionList.Empty();
}

void UActionLogicComponent::KeepAction()
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(0, 0.5, FColor::Red, FString::Printf(TEXT("keep Action: %s"), *CurrentAction->Name.ToString()));
    }
}

void UActionLogicComponent::ChangeAction(const FName& ActionId)
{
    FActionInfo* FoundAction = ActionList.FindByPredicate([&ActionId](const FActionInfo& Action)
                                                          { return Action.Name == ActionId; });
    if (!FoundAction)
    {
        FoundAction = CurrentAction;
    }
    if (!FoundAction)
    {
        return;
    }
    // Debug
    FActionInfo* LastAction = CurrentAction;

    CurrentAction           = FoundAction;
    CurrentActionFrameIndex = 0;
    CurrentBeCancelledTags.Empty();
    for (const auto& BeCancelledTag : CurrentAction->BeCancelledTags)
    {
        CurrentBeCancelledTags.Emplace(BeCancelledTag);
    }

    if (GEngine)
    {
        if (LastAction)
        {
            GEngine->AddOnScreenDebugMessage(1, 0.5, FColor::Blue, FString::Printf(TEXT("ChangeAction: Last:%s, Current:%s"), *LastAction->Name.ToString(), *CurrentAction->Name.ToString()));
        }
        else
        {
            GEngine->AddOnScreenDebugMessage(1, 0.5, FColor::Blue, FString::Printf(TEXT("ChangeAction: Current:%s"), *CurrentAction->Name.ToString()));
        }
    }
}

bool UActionLogicComponent::CanCancelCurrent(const FActionInfo& ActionInfo, int32 CheckFrame, bool CheckCommand, FBeCancelledTag& OutBeCancelledTag, FCancelTag& FoundTag)
{
    for (const auto& BeCancelledTag : CurrentBeCancelledTags)
    {
        if (!(BeCancelledTag.FrameRange.Min <= CheckFrame && BeCancelledTag.FrameRange.Max >= CheckFrame))
        {
            continue;
        }

        bool TagFound = false;
        for (const auto& Tag : BeCancelledTag.CancelTags)
        {
            for (const auto& CancelTag : ActionInfo.CancelTags)
            {
                if (CancelTag.Tag == Tag)
                {
                    TagFound          = true;
                    OutBeCancelledTag = BeCancelledTag;
                    FoundTag          = CancelTag;
                    break;
                }
            }
            if (TagFound)
            {
                break;
            }
        }
        if (!TagFound)
        {
            continue;
        }

        AACTGameCharacter* ch = Cast<AACTGameCharacter>(GetOwner());
        if (CheckCommand)
        {
            for (const auto& Command : ActionInfo.Commands)
            {
                if (ch->GetInputToCommandComponent()->ActionOccur(Command))
                {
                    return true;
                }
            }
        }
        else
        {
            return true;
        }
    }

    return false;
}

void UActionLogicComponent::SetCurrentActionInfoByIndex(int32 index)
{
    if (ActionList.IsValidIndex(index))
    {
        CurrentAction           = &ActionList[index];
        CurrentActionFrameIndex = 0;
        CurrentBeCancelledTags.Empty();
        for (const auto& BeCancelledTag : CurrentAction->BeCancelledTags)
        {
            CurrentBeCancelledTags.Emplace(BeCancelledTag);
        }
    }
}
