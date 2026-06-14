// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ACTGameGlobalFrameManager.h"
#include "ACTGlobalFrameManagerSystem.generated.h"

/**
 * 
 */
UCLASS()
class ACTGAME_API UACTGlobalFrameManagerSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	/** Override to control if the Subsystem should be created at all.
	 * For example you could only have your system created on servers.
	 * It's important to note that if using this is becomes very important to null check whenever getting the Subsystem.
	 *
	 * Note: This function is called on the CDO prior to instances being created!
	 *
	 */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const;

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection);

	/** Implement this for deinitialization of instances of the system */
	virtual void Deinitialize();

	friend struct FACTGameGlobalFrameManager;
private:
	void Tick(float DeltaTime);

	void RegisterTickFunction(class UWorld* world);
	void UnRegisterTickFunction();

public:
	void PostLoadMap(class UWorld* world);

public:
	UPROPERTY(EditDefaultsOnly, Category = Tick)
	struct FACTGameGlobalFrameManager Ticker;

	UFUNCTION(BlueprintCallable, Category = FrameCount)
	int64 GetCurrentFrame() const { return CurrentFrame; }

private:
	int64 CurrentFrame = 0;
	double NowTime = 0.0;
	double AccumulateTime = 0.0;
	const int32 FIXED_FPS = 60;
	float FixedDeltaTime = 1.f / FIXED_FPS;

};
