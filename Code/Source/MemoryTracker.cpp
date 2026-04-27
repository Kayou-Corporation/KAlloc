#include "MemoryTracker.hpp"
#include "Profiler.hpp"

#include <cstdio>
#include <cassert>          // assert


namespace Kayou::Memory
{
    void MemoryTracker::AddAllocation(const void* ptr, const std::size_t size, MemoryTag tag)
    {
        const std::size_t index = static_cast<std::size_t>(tag);

        const std::size_t previous = s_allocated[index].fetch_add(size);
        const std::size_t current = previous + size;
        std::size_t peak = s_peak[index];

        while (current > peak && !s_peak[index].compare_exchange_weak(peak, current)) { }

        // Calls tracy profiling if enabled in cmake
        Profiler::Alloc(ptr, size, GetTagName(tag));
    }


    void MemoryTracker::RemoveAllocation(const void* ptr, const std::size_t size, MemoryTag tag)
    {
        const std::size_t index = static_cast<std::size_t>(tag);

        [[maybe_unused]] const std::size_t previous = s_allocated[index].fetch_sub(size); // Thread-safe sub operation
        #ifdef KAYOU_DEBUG
        assert(previous >= size && "MemoryTracker::RemoveAllocation underflow");
        #else
        s_allocated[index] -= size;
        #endif

        Profiler::Free(ptr, GetTagName(tag)); // Calls tracy profiling if enabled in cmake
    }


    void MemoryTracker::ResetAll(MemoryTag tag)
    {
        const std::size_t index = static_cast<std::size_t>(tag);
        s_allocated[index] = 0;
        // do not reset s_peak to remember the peak mem usage
    }


    void MemoryTracker::PrintReport()
    {
        printf("==== Memory Report ====\n");

        for (size_t i = 0; i < static_cast<std::size_t>(MemoryTag::COUNT); ++i)
            printf("[%zu] Used: %zu bytes  -  Peak: %zu bytes\n\n", i, s_allocated[i].load(), s_peak[i].load());
    }


    const char* MemoryTracker::GetTagName(const MemoryTag tag)
    {
        switch (tag)
        {
            case MemoryTag::General:  return "General";
            case MemoryTag::Renderer: return "Renderer";
            case MemoryTag::Physics:  return "Physics";
            case MemoryTag::ECS:      return "ECS";
            case MemoryTag::Audio:    return "Audio";
            default:                  return "Unknown";
        }
    }
}