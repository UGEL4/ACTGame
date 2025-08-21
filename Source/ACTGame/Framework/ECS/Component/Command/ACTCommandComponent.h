#pragma once

#include <vector>
#include "Framework/ACTGameGameInstance.h"
#include "Framework/ECS/Entity/ACTGameEcsEntity.h"
#include "Framework/Game/Command/ActionCommand.h"

namespace ACTGame
{
    struct CommandComponent
    {
        std::vector<KeyInputRecord> Commands;
    };
}

namespace CommandSystem
{
    static void UpdateCommand(ACTGameEcsEntity* Entity, ACTGame::CommandComponent* CommandComponent)
    {
        /// 保持一个按键的存在时间最多这么多秒，太早的就释放掉了
        auto& GameInstance           = ACTGame::ACTGameInstance::Get();
        std::int64_t RecordKeepFrame = std::int64_t((1.2f * GameInstance.GetGameFrameRate()));
        auto& FrameManager           = GameInstance.GetFrameManager();
        std::int64_t CurrentFrame    = FrameManager.GetCurrentFrame();

        size_t index = 0;
        while (index < CommandComponent->Commands.size())
        {
            if (CurrentFrame - CommandComponent->Commands[index].Frame > RecordKeepFrame)
            {
                CommandComponent->Commands.erase(CommandComponent->Commands.begin() + index);
            }
            else
            {
                index++;
            }
        }
    }
}
