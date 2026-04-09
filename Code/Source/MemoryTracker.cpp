#include "MemoryTracker.hpp"

#include <cstdio>

#include "Profiler.hpp"



namespace Kayou::Memory
{
    void MemoryTracker::AddAllocation(const void* ptr, const std::size_t size, MemoryTag tag)
    {
        const std::size_t index = static_cast<std::size_t>(tag);

        s_allocated[index] += size;

        const std::size_t current = s_allocated[index];
        std::size_t peak = s_peak[index];

        while (current > peak && !s_peak[index].compare_exchange_weak(peak, current)) { }

        // Calls tracy profiling if enabled in cmake
        Profiler::Alloc(ptr, size, GetTagName(tag));
    }


    void MemoryTracker::RemoveAllocation(const void* ptr, const std::size_t size, MemoryTag tag)
    {
        const std::size_t index = static_cast<std::size_t>(tag);

        s_allocated[index] -= size;

        // Calls tracy profiling if enabled in cmake
        Profiler::Free(ptr, GetTagName(tag));
    }


    void MemoryTracker::ResetAll(MemoryTag tag)
    {
        const std::size_t index = static_cast<std::size_t>(tag);
        s_allocated[index] = 0;
        // do not reset s_peak in order to remember maximum
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