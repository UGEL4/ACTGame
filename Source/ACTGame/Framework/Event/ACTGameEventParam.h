#pragma once

#include "ACTGameEvent.h"
namespace ACTGame
{
struct EntityCreatedParam : EventParamHolder
{
    EntityCreatedParam(void* EntityPtr, void* ScenePtr)
        : EventParamHolder((int)EventType::EntityCreated)
        , Entity(EntityPtr)
        , Scene(ScenePtr)
    {
    }
    void* Entity;
    void* Scene;
};

struct SceneCreatedParam : EventParamHolder
{
    SceneCreatedParam(void* ScenePtr)
        : EventParamHolder((int)EventType::SceneCreated)
        , Scene(ScenePtr)
    {
    }
    void* Scene;
};
}
