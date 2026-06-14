// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Asset/ActionInfo/ActionInfoAsset.h"
#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Framework/Game/Action/CancelTag.h"
#include "HAL/Platform.h"
#include "Framework/Game/Action/ActionInfo.h"
#include "Framework/Game/Action/PreorderActionInfo.h"
#include "ActionLogicComponent.generated.h"


UCLASS( ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent) )
class ACTGAME_API UActionLogicComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UActionLogicComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void TickLogic(int64 CurrentFrame);

private:
	int64 CurrentLogicFrame = 0;

private:
	void UpdateAction();
	void KeepAction();
	UFUNCTION(BlueprintCallable)
	void ChangeAction(const FName& ActionId);
	bool CanCancelCurrent(const FActionInfo& ActionInfo, int32 CheckFrame, bool CheckCommand, FBeCancelledTag& OutBeCancelledTag, FCancelTag& FoundTag);

private:
	UPROPERTY(BlueprintReadWrite, Category = "Action Logic", meta = (AllowPrivateAccess = "true"))
	TArray<FActionInfo> ActionList;

	int32 CurrentActionFrameIndex = 0;

	FActionInfo* CurrentAction = nullptr;
	/**
	当前激活的BeCancelledTag
	 */
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TArray<FBeCancelledTag> CurrentBeCancelledTags;

	/**
	
	*/
	TArray<FPreorderActionInfo> PreorderActionList;

	FVector RootMotionMove = FVector::ZeroVector;
	FTransform RootMotionTransform = FTransform::Identity;
public:
	FVector GetRootMotionMove() const { return RootMotionMove; }
	const FTransform& GetRootMotionTransform() const { return RootMotionTransform; }

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void InitActionList(const TArray<UActionInfoAsset*>& ActionInfoAssetList);

	UFUNCTION(BlueprintCallable)
	void SetCurrentActionInfoByIndex(int32 index);
};
