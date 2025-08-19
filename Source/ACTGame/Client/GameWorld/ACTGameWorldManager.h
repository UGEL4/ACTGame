
#pragma once

#include "CoreMinimal.h"
#include "Framework/ECS/Scene/ACTGameEcsScene.h"
#include "Framework/ECS/Scene/ACTGameEcsSceneManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Framework/Event/ACTGameEvent.h"
#include "Framework/Event/ACTGameEventParam.h"
#include "ACTGameWorldManager.generated.h"


class ACTGameEcsScene;
/**
 * 
 */
UCLASS()
class ACTGAME_API UACTGameWorldManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
    UACTGameWorldManager();
    ~UACTGameWorldManager();

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

public:
	void PostLoadMap(class UWorld* world);

private:
    void OnLogicSceneCreate(ACTGameEcsScene* NewScene);

    UFUNCTION(BlueprintCallable, Category = "ACTGame|World")
    void StartGame();

private:
    TSharedPtr<ACTGameEcsSceneManager> SceneManager;

	ACTGame::EventHandler<ACTGame::SceneCreatedParam> SceneCreatedCallbackHandler;

};
