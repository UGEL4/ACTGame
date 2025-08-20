#pragma once

#include <memory>
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
    std::shared_ptr<ACTGameEcsSceneManager> SceneManager;
};
}
