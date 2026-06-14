#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ACTGameGlobalFrameManager.generated.h"

/**
* Tick function that calls AActor::TickActor
**/
USTRUCT()
struct FACTGameGlobalFrameManager : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	class UACTGlobalFrameManagerSystem* Target;

//	/**  AActor  that is the target of this tick **/
//#if UE_WITH_REMOTE_OBJECT_HANDLE
//	TObjectPtr<AActor> Target;
//#else
//	class AActor* Target;
//#endif

	/**
		* Abstract function actually execute the tick.
		* @param DeltaTime - frame time to advance, in seconds
		* @param TickType - kind of tick for this frame
		* @param CurrentThread - thread we are executing on, useful to pass along as new tasks are created
		* @param MyCompletionGraphEvent - completion event for this task. Useful for holding the completetion of this task until certain child tasks are complete.
	**/
	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;
	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	virtual FString DiagnosticMessage() override;
	virtual FName DiagnosticContext(bool bDetailed) override;

//#if UE_WITH_REMOTE_OBJECT_HANDLE
//	/** Exposes references to GC system */
//	ENGINE_API void AddStructReferencedObjects(FReferenceCollector& Collector);
//#endif
};

template<>
struct TStructOpsTypeTraits<FACTGameGlobalFrameManager> : public TStructOpsTypeTraitsBase2<FACTGameGlobalFrameManager>
{
	enum
	{
//#if UE_WITH_REMOTE_OBJECT_HANDLE
//		WithAddStructReferencedObjects = true,
//#endif
		WithCopy = false
	};
};
