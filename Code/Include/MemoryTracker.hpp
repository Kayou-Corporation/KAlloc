#pragma once

#include <array>
#include <atomic>
#include <cstdint>



namespace Kayou
{
    enum class MemoryTag : uint8_t
    {
        General     =   0,
        Renderer    =   1,
        Physics     =   2,
        ECS         =   3,
        Audio       =   4,
        COUNT
    };



    class MemoryTracker
    {
    public:
        static void AddAllocation(std::size_t size, MemoryTag tag = MemoryTag::General);
        static void RemoveAllocation(std::size_t size, MemoryTag tag = MemoryTag::General);
        static void PrintReport();


    private:
        inline static std::array<std::atomic<std::size_t>, static_cast<size_t>(MemoryTag::COUNT)> s_allocated { };
        inline static std::array<std::atomic<std::size_t>, static_cast<size_t>(MemoryTag::COUNT)> s_peak { };
    };
}
