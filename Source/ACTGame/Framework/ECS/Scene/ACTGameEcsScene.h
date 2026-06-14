#pragma once

#include "CoreMinimal.h"
#include "entity/fwd.hpp"
#include "entt.hpp"
#include <vector>

class ACTGameEcsEntity;

class ACTGAME_API ACTGameEcsScene
{
public:
    ACTGameEcsScene();
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
private:
    entt::registry Registry;
    std::vector<class ACTGameEcsEntity*> Entities;
    class ACTGameEcsEntity* SceneEntity {nullptr};
};

