#include "MemoryAllocator.hpp"

#include <algorithm>



namespace Kayou
{
    void* FreeableAllocator::Realloc(void* ptr, const std::size_t newSize)
    {
        return Realloc(ptr, newSize, alignof(std::max_align_t));
    }


    void* MemoryAllocator::Alloc(const std::size_t size)
    {
        return Alloc(size, alignof(std::max_align_t));
    }


    void MemoryAllocator::TrackAlloc(const std::size_t size)
    {
        p_usedSize += size;
        p_peakSize = std::max(p_peakSize, p_usedSize);
    }


    void MemoryAllocator::TrackFree(const std::size_t size)
    {
        p_usedSize = (p_usedSize >= size) ? (p_usedSize - size) : 0;
    }
}