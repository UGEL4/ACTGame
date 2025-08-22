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
    int a = 0;
}

ACTGameEcsScene* ACTGameEcsSceneManager::CreateScene(std::string_view SceneName)
{
    ACTGameEcsScene* NewScene = (ACTGameEcsScene*)ACTGameGlobal::NewObject<ACTGameEcsScene>(SceneName);
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
            ACTGameGlobal::DeleteObject(Scene);
        }
    }
    ActiveScene = nullptr;
    SceneList.clear();
}

void ACTGameEcsSceneManager::Update(float DeltaTime, std::int64_t Frame)
{
    if (ActiveScene)
    {
        ActiveScene->Update(DeltaTime, Frame);
    }
}
