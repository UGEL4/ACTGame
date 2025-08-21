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

        FrameManager.SetGameFrameRate(GameFrameRate);
        FrameManager.SetTickCallback(std::bind(&ACTGameInstance::GameLoop, this, std::placeholders::_1, std::placeholders::_2));
    }

    void ACTGameInstance::Shutdown()
    {
        SceneManager->ClearAllScene();
    }

    void ACTGameInstance::SetLogger(std::unique_ptr<ILogService, ACTGameGlobal::DefaultDeleter> logger)
    {
        Logger = std::move(logger);
    }

    void ACTGameInstance::GameLoop(float DeltaTime, std::int64_t Frame)
    {
        if (SceneManager)
        {
            SceneManager->Update(DeltaTime, Frame);
        }
    }
}// namespace ACTGame
