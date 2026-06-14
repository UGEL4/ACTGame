#pragma once

#include "ACTGameEcsScene.h"
#include <vector>

class ACTGameEcsScene;

class ACTGameEcsSceneManager
{
public:
    ACTGameEcsSceneManager();
    ~ACTGameEcsSceneManager();

public:
    ACTGameEcsScene* CreateScene();
    /* 销毁之前要调用 */
    void ClearAllScene();

    ACTGameEcsScene* GetCurrentActiveScene() const { return ActiveScene; }
private:
    std::vector<ACTGameEcsScene*> SceneList;
    ACTGameEcsScene* ActiveScene{ nullptr };
};
