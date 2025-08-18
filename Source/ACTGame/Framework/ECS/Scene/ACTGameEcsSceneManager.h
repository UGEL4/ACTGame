#pragma once

#include "ACTGameEcsScene.h"
#include <vector>
#include <string_view>

class ACTGameEcsScene;

class ACTGameEcsSceneManager
{
public:
    ACTGameEcsSceneManager();
    ~ACTGameEcsSceneManager();

public:
    ACTGameEcsScene* CreateScene(std::string_view SceneName);
    /* 销毁之前要调用 */
    void ClearAllScene();

    ACTGameEcsScene* GetCurrentActiveScene() const { return ActiveScene; }
private:
    std::vector<ACTGameEcsScene*> SceneList;
    ACTGameEcsScene* ActiveScene{ nullptr };
};
