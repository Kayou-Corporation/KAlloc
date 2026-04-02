#include "MemoryTracker.hpp"

#include <cstdio>


void Kayou::MemoryTracker::AddAllocation(const std::size_t size, MemoryTag tag)
{
    s_allocated[static_cast<std::size_t>(tag)] += size;

    std::size_t current = s_allocated[static_cast<std::size_t>(tag)];
    std::size_t peak = s_peak[static_cast<std::size_t>(tag)];

    while (current > peak && !s_peak[static_cast<std::size_t>(tag)].compare_exchange_weak(peak, current)) { }
}


void Kayou::MemoryTracker::RemoveAllocation(const std::size_t size, MemoryTag tag)
{
    s_allocated[static_cast<std::size_t>(tag)] -= size;
}


void Kayou::MemoryTracker::PrintReport()
{
    for (size_t i = 0; i < static_cast<std::size_t>(MemoryTag::COUNT); ++i)
        printf("[%zu] Used: %zu bytes  -  Peak: %zu bytes\n", i, s_allocated[i].load(), s_peak[i].load());
}
