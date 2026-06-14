#include "ACTGameEcsEntity.h"

ACTGameEcsEntity::ACTGameEcsEntity()
{
    
}

ACTGameEcsEntity::ACTGameEcsEntity(entt::entity InHandle, ACTGameEcsScene* InScene)
 : Scene(InScene), Handle(InHandle)
{

}

ACTGameEcsEntity::~ACTGameEcsEntity()
{
    Scene = nullptr;
}
