#pragma once

#include <vector>

namespace ACTGame
{
    enum class KeyMap : uint8
    {
        NoDir,
        NoInput,
        DirInput,
        ButtonWest,
        ButtonEast,
        ButtonNouth,
        ButtonSouth
    };

    struct KeyInputRecord
    {
        KeyMap Key;
        //frame while record
        std::int64_t Frame;
    };
} // namespace ACTGame
