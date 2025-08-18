#pragma once

#include "CoreMinimal.h"
#include "Framework/ECS/Scene/ACTGameEcsScene.h"
#include "entity/fwd.hpp"

class ACTGAME_API ACTGameEcsEntity
{
public:
    ACTGameEcsEntity();
    ACTGameEcsEntity(entt::entity InHandle, ACTGameEcsScene* InScene);
    ~ACTGameEcsEntity();

    friend class ACTGameEcsScene;
public:
    template <typename T, typename ... Args>
    T& AddComponent(Args&& ... args)
    {
        return Scene->Registry.emplace<T>(Handle, std::forward<Args>(args)...);
    }

    template <typename T>
    std::size_t RemoveComponent()
    {
        Scene->Registry.remove<T>(Handle);
    }

    template <typename T>
    T& GetComponent()
    {
        return Scene->Registry.get<T>(Handle);
    }

private:
    ACTGameEcsScene* Scene{ nullptr };
    entt::entity Handle {static_cast<entt::entity>(0)};
};
