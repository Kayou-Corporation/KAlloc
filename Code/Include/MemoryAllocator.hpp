#pragma once

#include <cstddef>



namespace Kayou
{
    class MemoryAllocator
    {
    public:
        virtual ~MemoryAllocator() = default;

        virtual void* Alloc(std::size_t size, std::size_t memAlignment) = 0;
        virtual void PrintUsage() const = 0;

        void* Alloc(std::size_t size);

        std::size_t GetTotalSize() const { return p_totalSize; };
        std::size_t GetUsedSize() const { return p_usedSize; };
        std::size_t GetPeakSize() const { return p_peakSize; };


    protected:
        void TrackAlloc(std::size_t size);
        void TrackFree(std::size_t size);

        std::size_t p_totalSize = 0;
        std::size_t p_usedSize = 0;
        std::size_t p_peakSize = 0;
    };



    class FreeableAllocator : MemoryAllocator
    {
    public:
        virtual void Free(void* ptr) = 0;
        virtual void* Realloc(void* ptr, std::size_t newSize, std::size_t memAlignment) = 0;

        void* Realloc(void* ptr, std::size_t newSize);
    };
}
