#pragma once

#include <array>
#include <atomic>
#include <cstdint>      // GCC std::uint32_t & std::uintptr_t



namespace Kayou::Memory
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

        /// Internal function called by TrackedAllocator, used to register an allocation internally
        /// @param ptr The pointer to allocate
        /// @param size The allocation block size
        /// @param tag [OPTIONAL] A tag used to differentiate allocation blocs - Defaults to MemoryTag::General
        static void AddAllocation(const void* ptr, std::size_t size, MemoryTag tag = MemoryTag::General);

        /// Internal function called by TrackedAllocator, used to unregister an allocation internally
        /// @param ptr The pointer to deallocate
        /// @param size The allocation block size
        /// @param tag [OPTIONAL] A tag used to differentiate allocation blocs - Defaults to MemoryTag::General
        static void RemoveAllocation(const void* ptr, std::size_t size, MemoryTag tag = MemoryTag::General);

        /// Internal function called by TrackedAllocator, used to register all allocations internally
        /// @param tag [OPTIONAL] A tag used to differentiate allocation blocs - Defaults to MemoryTag::General
        static void ResetAll(MemoryTag tag = MemoryTag::General);

        /// Internal function used to print the allocation report into the console
        static void PrintReport();

        /// Internal helper function used to convert a MemoryTag into a char* in order to be forwarded to Tracy
        static const char* GetTagName(MemoryTag tag);


    private:
        inline static std::array<std::atomic<std::size_t>, static_cast<size_t>(MemoryTag::COUNT)> s_allocated { };
        inline static std::array<std::atomic<std::size_t>, static_cast<size_t>(MemoryTag::COUNT)> s_peak { };
    };
}
