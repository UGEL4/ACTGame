#pragma once

#include "CoreMinimal.h"
#include "entity/fwd.hpp"
#include "entt.hpp"
#include <vector>
#include <string_view>

class ACTGameEcsEntity;

class ACTGAME_API ACTGameEcsScene
{
public:
    ACTGameEcsScene();
    ACTGameEcsScene(std::string_view SceneName);
    ~ACTGameEcsScene();

public:
    friend class ACTGameEcsEntity;
    ACTGameEcsEntity* CreateEntity();

    void Initialize();
    void Deinitialize();

public:
    void Update(float DeltaTame, int64 Frame);
public:
    void OnSceneViewLoaded();
    ACTGameEcsEntity* GetSceneEntity() const { return SceneEntity; }
private:
    entt::registry Registry;
    std::vector<class ACTGameEcsEntity*> Entities;
    class ACTGameEcsEntity* SceneEntity {nullptr};
    std::string_view SceneName;
};

