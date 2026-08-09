// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FrameWork/Game/Command/ActionCommand.h"
#include "HAL/Platform.h"
#include "Math/MathFwd.h"
#include "UObject/ObjectMacros.h"
#include "InputToCommandComponent.generated.h"

USTRUCT()
struct FKeyInputRecord
{
	GENERATED_BODY()

	int32 FrameIndex = 0;
	EKeyMap Key = EKeyMap::NoDir;
	FVector2D DirInput = FVector2D::ZeroVector;

	FKeyInputRecord() = default;
	FKeyInputRecord(int32 InFrameIndex, EKeyMap InKey, FVector2D InDirInput = FVector2D::ZeroVector)
		: FrameIndex(InFrameIndex)
		, Key(InKey)
		, DirInput(InDirInput)
	{
	}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ACTGAME_API UInputToCommandComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInputToCommandComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void TickLogic(int64 CurrentFrame);

	void TryAddInput(EKeyMap Key);
	void TryAddDirectInput(FVector2D Input);
	bool ActionOccur(const FActionCommand& Command);
    bool ActionOccur(const FActionCommand& Command, FKeyInputRecord& OutData);

private:
	void UpdateCommand();
	int64 CurrentLogicFrame = 0;

private:
	/// <summary>
    /// 保持一个按键的存在时间最多这么多秒，太早的就释放掉了
    /// </summary>
    const int32 RecordKeepFrame = 1.2 * 60;
    
    /// <summary>
    /// 当前的按键记录
    /// </summary>
    TArray<FKeyInputRecord> Inputs;

    /// <summary>
    /// 非方向输入得有个cd，不然频率太高影响连招手感
    /// 如果没有这个，非常容易出现“全自动连续攻击”，如果攻击派生动作的按键一样的话
    /// </summary>
    const int32 NonDirectionInputCooldown = 0.09 * 60;

    int32 NonDirectCooldown = 0;
	bool PendingCooldown = false;
	bool NoDirInput      = true;
};
