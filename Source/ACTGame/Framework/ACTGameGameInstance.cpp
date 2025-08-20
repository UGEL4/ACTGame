#include "ACTGameGameInstance.h"
#include "Client/Log/UELog.h"
#include "Misc/ACTGameMemory.h"
#include <memory>

namespace ACTGame
{
    void ACTGameInstance::Init()
    {
        ACTGameEcsSceneManager* ECSSceneManager = ACTGameGlobal::NewObject<ACTGameEcsSceneManager>();
        SceneManager = std::shared_ptr<ACTGameEcsSceneManager>(ECSSceneManager, ACTGameGlobal::DefaultDeleter());
    }

    void ACTGameInstance::Shutdown()
    {
        SceneManager->ClearAllScene();
    }

    void ACTGameInstance::SetLogger(std::unique_ptr<ILogService, ACTGameGlobal::DefaultDeleter> logger)
    {
        Logger = std::move(logger);
    }
}// namespace ACTGame
