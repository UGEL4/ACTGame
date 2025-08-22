#pragma once

#include <memory>
#include "ACTGameFrameManager.h"
#include "Framework/ECS/Scene/ACTGameEcsSceneManager.h"
#include "Framework/Interface/ILogService.h"
#include "Misc/ACTGameMemory.h"
namespace ACTGame
{
class ACTGameInstance final
{
public:
    ACTGameInstance(const ACTGameInstance&) = delete;
    ACTGameInstance& operator=(const ACTGameInstance&) = delete;
private:
    ACTGameInstance() = default;
    ~ACTGameInstance();
public:
    static ACTGameInstance& Get()
    {
        static ACTGameInstance Instance;
        return Instance;
    }

public:
    void Init();
    void Shutdown();
    
public:
    void SetLogger(std::unique_ptr<ILogService, ACTGameGlobal::DefaultDeleter> logger);
private:
    std::unique_ptr<ILogService, ACTGameGlobal::DefaultDeleter> Logger;

public:
    ACTGameEcsSceneManager *GetSceneManager() const { return SceneManager.get(); }

private:
    std::shared_ptr<ACTGameEcsSceneManager> SceneManager {nullptr};

/**************************************************************************************/
public:
    void SetGameFrameRate(std::int32_t FrameRate)
    {
        GameFrameRate = FrameRate;
    }

    std::int32_t GetGameFrameRate() const { return GameFrameRate; }

    const ACTGameFrameManager& GetFrameManager() const { return FrameManager; }
    ACTGameFrameManager& GetFrameManager() { return FrameManager; }

private:
    void GameLoop(float DeltaTime, std::int64_t Frame);
    std::int32_t GameFrameRate{60};
    ACTGameFrameManager FrameManager;
    /**************************************************************************************/
};
}
