#include "ACTGameWorldManager.h"
#include "Framework/ECS/Component/ACTGameTransformComponent.h"
#include "Framework/ECS/Scene/ACTGameEcsScene.h"
#include "Templates/SharedPointer.h"
#include "Framework/ECS/Scene/ACTGameEcsSceneManager.h"
#include "Framework/ECS/Scene/ACTGameEcsScene.h"

UACTGameWorldManager::UACTGameWorldManager()
{
    SceneManager = MakeShared<ACTGameEcsSceneManager>();
}

UACTGameWorldManager::~UACTGameWorldManager()
{
    
}

bool UACTGameWorldManager::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void UACTGameWorldManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //创建默认场景
    SceneManager->CreateScene("MainMenuLevel");

    //注册场景创建的回调
    TWeakObjectPtr<UACTGameWorldManager> WeakThis(this);
    ACTGame::EventService EventService;
    SceneCreatedCallbackHandler.EventId = ACTGame::EventType::SceneCreated;
	SceneCreatedCallbackHandler.Handler = EventService.RegisterHandler<ACTGame::SceneCreatedParam>((int)ACTGame::EventType::SceneCreated, [WeakThis](const ACTGame::SceneCreatedParam& param)
	{
        if (UACTGameWorldManager* ValidThis = WeakThis.Get())
		{
            UE_LOG(LogTemp, Warning, TEXT("创建了scene实体: scene:%lld"), (size_t)param.Scene);
            ValidThis->OnLogicSceneCreate((ACTGameEcsScene*)param.Scene);
        }
	});

    // 绑定地图加载完成事件
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UACTGameWorldManager::PostLoadMap);
}

void UACTGameWorldManager::Deinitialize()
{
    ACTGame::EventService EventService;
    EventService.RemoveHandler(SceneCreatedCallbackHandler);
    if (SceneManager)
    {
        SceneManager->ClearAllScene();
    }
}

void UACTGameWorldManager::PostLoadMap(UWorld* world)
{
    if (SceneManager)
    {
        auto scene = SceneManager->GetCurrentActiveScene();
        if (scene)
        {
            scene->OnSceneViewLoaded();
        }
    }
}

void UACTGameWorldManager::OnLogicSceneCreate(ACTGameEcsScene* NewScene)
{

}
