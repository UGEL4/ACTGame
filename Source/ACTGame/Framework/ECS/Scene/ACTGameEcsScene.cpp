#include "ACTGameEcsScene.h"
#include "Framework/ECS/Entity/ACTGameEcsEntity.h"
#include "Framework/ECS/Component/ACTGameTransformComponent.h"
#include "Framework/ECS/Entity/ACTGameEcsEntity.h"
#include "Framework/Misc/ACTGameMemory.h"
#include <iostream>

ACTGameEcsScene::ACTGameEcsScene()
{
    std::cout << "ACTGameEcsScene: " << this << std::endl;
}

ACTGameEcsScene::ACTGameEcsScene(std::string_view Name)
: SceneName(Name)
{

}

ACTGameEcsScene::~ACTGameEcsScene()
{
    std::cout << "~ACTGameEcsScene: " << this << std::endl;
}

ACTGameEcsEntity* ACTGameEcsScene::CreateEntity()
{
    ACTGameEcsEntity* EcsEntity = (ACTGameEcsEntity*)ACTGameGlobal::NewObject<ACTGameEcsEntity>(Registry.create(), this);
    //EcsEntity->Scene            = this;
    //EcsEntity->Handle           = Registry.create();
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
    SceneEntity         = (ACTGameEcsEntity*)ACTGameGlobal::NewObject<ACTGameEcsEntity>();
    SceneEntity->Scene  = this;
    SceneEntity->Handle = Registry.create();
    SceneEntity->AddComponent<ACTGame::PositionComponent>(0.f, 0.f, 0.f);
    SceneEntity->AddComponent<ACTGame::RotationComponent>(0.f, 0.f, 0.f, 1.f);
    SceneEntity->AddComponent<ACTGame::ScaleComponent>(1.f, 1.f, 1.f);
    if (!SceneName.empty())
    {
        SceneEntity->AddComponent<ACTGame::NameComponent>(SceneName);
    }
}

void ACTGameEcsScene::Deinitialize()
{
    if (SceneEntity)
    {
        ACTGameGlobal::DeleteObject(SceneEntity);
        SceneEntity = nullptr;
    }
    for (auto EcsEntity : Entities)
    {
        if (EcsEntity != nullptr)
        {
            ACTGameGlobal::DeleteObject(EcsEntity);
        }
    }
    Entities.clear();
}
