#include "ACTGameEcsEntity.h"
#include <iostream>

ACTGameEcsEntity::ACTGameEcsEntity()
{
    std::cout << "ACTGameEcsEntity: " << this << std::endl;
}

ACTGameEcsEntity::ACTGameEcsEntity(entt::entity InHandle, ACTGameEcsScene* InScene)
 : Scene(InScene), Handle(InHandle)
{
    std::cout << "ACTGameEcsEntity: " << this << std::endl;
}

ACTGameEcsEntity::~ACTGameEcsEntity()
{
    std::cout << "~ACTGameEcsEntity: " << this << std::endl;
    Scene = nullptr;
}
