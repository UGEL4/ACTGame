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

namespace ACTGameGlobal
{
template <typename T, typename... Args>
T* NewObject(Args&&... args)
{
    size_t Size = sizeof(T);
    void* Ptr   = ACTGame::ACTGameMemory::Malloc(Size);
    T* Obj      = new (Ptr) T(std::forward<Args>(args)...);
    return Obj;
}

template <typename T>
void DeleteObject(T* Obj)
{
    Obj->~T();
    ACTGame::ACTGameMemory::Free(Obj);
}
} // namespace ACTGameGlobal
