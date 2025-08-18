#include "ACTGameGlobalFrameManager.h"
#include "ACTGlobalFrameManagerSystem.h"

void FACTGameGlobalFrameManager::ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (IsValid(Target))
	{
		//if (TickType != LEVELTICK_ViewportsOnly || Target->ShouldTickIfViewportsOnly())
		//{
		FScopeCycleCounterUObject ActorScope(Target);
		Target->Tick(DeltaTime);
		//}
	}
}

/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
FString FACTGameGlobalFrameManager::DiagnosticMessage()
{
	return TEXT("[FACTGameGlobalFrameManagerTick]");
}

FName FACTGameGlobalFrameManager::DiagnosticContext(bool bDetailed)
{
	return FName(TEXT("FACTGameGlobalFrameManager"));
}