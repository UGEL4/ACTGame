#include "ACTGameEcsSceneManager.h"
#include "Framework/Event/ACTGameEvent.h"
#include "Framework/Event/ACTGameEventParam.h"
#include "Framework/Misc/ACTGameMemory.h"
#include "ACTGameEcsScene.h"

ACTGameEcsSceneManager::ACTGameEcsSceneManager()
{
}

ACTGameEcsSceneManager::~ACTGameEcsSceneManager()
{
}

ACTGameEcsScene* ACTGameEcsSceneManager::CreateScene()
{
    size_t Size               = sizeof(ACTGameEcsScene);
    ACTGameEcsScene* NewScene = (ACTGameEcsScene*)ACTGame::ACTGameMemory::Malloc(Size);
    NewScene->Initialize();
    if (!ActiveScene)
    {
        ActiveScene = NewScene;
    }
    SceneList.emplace_back(NewScene);
    ////发布事件
    ACTGame::EventService EventService;
    EventService.Emit((int)ACTGame::EventType::SceneCreated, ACTGame::SceneCreatedParam(NewScene));
    return NewScene;
}

void ACTGameEcsSceneManager::ClearAllScene()
{
    for (auto Scene : SceneList)
    {
        if (Scene)
        {
            Scene->Deinitialize();
            ACTGame::ACTGameMemory::Free(Scene);
        }
    }
    ActiveScene = nullptr;
    SceneList.clear();
}
