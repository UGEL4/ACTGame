#include "ACTGameWorldManager.h"
#include "ACTGameInstance.h"
#include "Framework/ACTGameGameInstance.h"
#include "Framework/ECS/Component/ACTGameTransformComponent.h"
#include "Framework/ECS/Entity/ACTGameEcsEntity.h"
#include "Framework/Event/ACTGameEventParam.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/ECS/Scene/ACTGameEcsSceneManager.h"
#include "Engine/World.h"

UACTGameWorldManager::UACTGameWorldManager()
{

}

UACTGameWorldManager::~UACTGameWorldManager()
{
    
}

bool UACTGameWorldManager::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void UACTGameWorldManager::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    UACTGameInstance* GameInstance = Cast<UACTGameInstance>(GetGameInstance());
    if (!GameLoopTickerHandle.IsValid())
    {
        uint32 GameFrameRate = GameInstance->GetGameFrameRate();
        GameLoopTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UACTGameWorldManager::GameLoop), 1.f / GameFrameRate);
    }

    // 注册场景创建的回调
    TWeakObjectPtr<UACTGameWorldManager> WeakThis(this);
    ACTGame::EventService EventService;
    SceneCreatedCallbackHandler.EventId = ACTGame::EventType::SceneCreated;
    SceneCreatedCallbackHandler.Handler = EventService.RegisterHandler<ACTGame::SceneCreatedParam>(
        (int)ACTGame::EventType::SceneCreated,
        [WeakThis](const ACTGame::SceneCreatedParam &param)
        {
            if (UACTGameWorldManager *ValidThis = WeakThis.Get())
            {
                // UE_LOG(LogTemp, Warning, TEXT("创建了scene实体: scene:%lld"), (size_t)param.Scene);
                ValidThis->OnLogicSceneCreate((ACTGameEcsScene *)param.Scene);
            }
        });

    EntityCreatedCallbackHandler.EventId = ACTGame::EventType::EntityCreated;
    EntityCreatedCallbackHandler.Handler = EventService.RegisterHandler<ACTGame::EntityCreatedParam>(
        (int)ACTGame::EventType::EntityCreated,
        [WeakThis](const ACTGame::EntityCreatedParam &param)
        {
            if (UACTGameWorldManager *ValidThis = WeakThis.Get())
            {
                // UE_LOG(LogTemp, Warning, TEXT("创建了scene实体: scene:%lld"), (size_t)param.Scene);
                //ValidThis->OnLogicSceneCreate((ACTGameEcsScene *)param.Scene);
                FSpawnCharacterEventParam Param;
                Param.Entity = (ACTGameEcsEntity*)param.Entity;
                ValidThis->OnCreateCharacter.Broadcast(Param);
            }
        });

    // 绑定地图加载完成事件
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UACTGameWorldManager::PostLoadMap);
}

void UACTGameWorldManager::Deinitialize()
{
    if (GameLoopTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(GameLoopTickerHandle);
    }
    ACTGame::EventService EventService;
    EventService.RemoveHandler(SceneCreatedCallbackHandler);
}

void UACTGameWorldManager::PostLoadMap(UWorld* world)
{
    ACTGame::ACTGameInstance& GameInstance = ACTGame::ACTGameInstance::Get();
    auto SceneManager = GameInstance.GetSceneManager();
    if (SceneManager)
    {
        auto scene = SceneManager->GetCurrentActiveScene();
        if (scene)
        {
            scene->OnSceneViewLoaded();

            //测试
            ACTGameEcsEntity* NewEntity = scene->CreateEntity();
            ////发布事件
            ACTGame::EventService EventService;
            EventService.Emit((int)ACTGame::EventType::EntityCreated, ACTGame::EntityCreatedParam(NewEntity, scene));
        }
    }
}

void UACTGameWorldManager::OnLogicSceneCreate(ACTGameEcsScene* NewScene)
{
    ACTGame::NameComponent& comp = NewScene->GetSceneEntity()->GetComponent<ACTGame::NameComponent>();
    FName SceneName = comp.Name.data();
    UGameplayStatics::OpenLevel(GetGameInstance()->GetWorld(), SceneName);
    UE_LOG(LogTemp, Warning, TEXT("创建了scene实体: scene:%lld name:%s"), (size_t)NewScene, *SceneName.ToString());
}

void UACTGameWorldManager::StartGame()
{
    auto SceneManager = ACTGame::ACTGameInstance::Get().GetSceneManager();
    //UGameplayStatics::OpenLevel(GetGameInstance()->GetWorld(), TEXT("Lvl_ThirdPerson"));
    ACTGameEcsScene* NewScene = SceneManager->CreateScene("Lvl_ThirdPerson");
}

bool UACTGameWorldManager::GameLoop(float DeltaTime)
{
    return true;
}
