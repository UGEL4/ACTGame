#include "ACTGameEcsScene.h"
#include "Framework/ECS/Entity/ACTGameEcsEntity.h"
#include "Framework/ECS/Component/ACTGameTransformComponent.h"
#include "Framework/ECS/Entity/ACTGameEcsEntity.h"
#include "Framework/Misc/ACTGameMemory.h"

ACTGameEcsScene::ACTGameEcsScene()
{
    
}

ACTGameEcsScene::~ACTGameEcsScene()
{
    
}

ACTGameEcsEntity* ACTGameEcsScene::CreateEntity()
{
    size_t Size                 = sizeof(ACTGameEcsEntity);
    ACTGameEcsEntity* EcsEntity = (ACTGameEcsEntity*)ACTGame::ACTGameMemory::Malloc(Size);
    EcsEntity->Scene            = this;
    EcsEntity->Handle           = Registry.create();
    Entities.emplace_back(EcsEntity);
    return EcsEntity;
}

void ACTGameEcsScene::Update(float DeltaTame, int64 Frame)
{
    
}

void ACTGameEcsScene::OnSceneViewLoaded()
{

}

void ACTGameEcsScene::Initialize()
{
    size_t Size         = sizeof(ACTGameEcsEntity);
    SceneEntity         = (ACTGameEcsEntity*)ACTGame::ACTGameMemory::Malloc(Size);
    SceneEntity->Scene  = this;
    SceneEntity->Handle = Registry.create();
    SceneEntity->AddComponent<ACTGame::PositionComponent>(0.f, 0.f, 0.f);
    SceneEntity->AddComponent<ACTGame::RotationComponent>(0.f, 0.f, 0.f, 1.f);
    SceneEntity->AddComponent<ACTGame::ScaleComponent>(1.f, 1.f, 1.f);
}

void ACTGameEcsScene::Deinitialize()
{
    if (SceneEntity)
    {
        ACTGame::ACTGameMemory::Free(SceneEntity);
        SceneEntity = nullptr;
    }
    for (auto EcsEntity : Entities)
    {
        if (EcsEntity != nullptr)
        {
            ACTGame::ACTGameMemory::Free(EcsEntity);
        }
    }
    Entities.clear();
}
