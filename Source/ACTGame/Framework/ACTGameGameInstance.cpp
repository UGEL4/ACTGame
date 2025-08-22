#include "ACTGameGameInstance.h"
#include "Client/Log/UELog.h"
#include "Misc/ACTGameMemory.h"
#include <memory>

namespace ACTGame
{
    ACTGameInstance::~ACTGameInstance()
    {
        int a = 0;
    }

    void ACTGameInstance::Init()
    {
        ACTGameEcsSceneManager* ECSSceneManager = ACTGameGlobal::NewObject<ACTGameEcsSceneManager>();
        SceneManager = std::shared_ptr<ACTGameEcsSceneManager>(ECSSceneManager, ACTGameGlobal::DefaultDeleter());
        
        FrameManager.Reset();
        FrameManager.SetGameFrameRate(GameFrameRate);
        FrameManager.SetTickCallback(std::bind(&ACTGameInstance::GameLoop, this, std::placeholders::_1, std::placeholders::_2));
    }

    void ACTGameInstance::Shutdown()
    {
        SceneManager->ClearAllScene();
        SceneManager.reset();
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
