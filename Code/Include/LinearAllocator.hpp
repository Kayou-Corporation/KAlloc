#pragma once

#include "MemoryAllocator.hpp"



namespace Kayou
{
    class LinearAllocator
    {
    public:
        explicit LinearAllocator(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        ~LinearAllocator();

        void* Alloc(std::size_t size, std::size_t memAlignment);
        void Reset();
        void PrintUsage() const;



    private:
        static std::size_t AlignForward(std::size_t ptrAddress, std::size_t memAlignment);

        void TrackAlloc(std::size_t size);

        std::byte* m_start = nullptr;
        std::size_t m_offset = 0;

        std::size_t m_totalSize = 0;
        std::size_t m_usedSize = 0;
        std::size_t m_peakSize = 0;
    };
}