#pragma once

#include "HAL/UnrealMemory.h"
namespace ACTGame
{
struct ACTGameMemory
{
    static constexpr std::uint32_t DEFAULT_ALIGMENT = 16;
    static void* Malloc(std::size_t Size, std::uint32_t Aligment = DEFAULT_ALIGMENT)
    {
        return FMemory::Malloc(Size);
    }

    static void Free(void* Ptr)
    {
        if (nullptr == Ptr)
        {
            return;
        }
        FMemory::Free(Ptr);
    }
};

} // namespace ACTGame
