#pragma once
#include <cstddef>

#include "Utils/Utils.hpp"


namespace Kayou::Memory
{
    class StackAllocator
    {
    public:
        explicit StackAllocator(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        ~StackAllocator();

        StackAllocator(const StackAllocator&) = delete;
        StackAllocator(StackAllocator&&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;
        StackAllocator& operator=(StackAllocator&&) = delete;

        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        void Free(void* ptr);
        void Reset();
        void PrintUsage() const;

        KAYOU_ALWAYS_INLINE void Pop(void* ptr) { Free(ptr); }

        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetUsedSize() const    { return m_usedSize; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetPeakSize() const    { return m_peakSize; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetTotalSize() const   { return m_totalSize; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetOffset() const      { return m_offset; }


    private:
        #ifdef KAYOU_DEBUG
        const std::uint32_t kStackAllocationHeaderMagic = 0xBAADF00Du;
        #endif

        struct AllocationHeader
        {
            std::size_t previousOffset;
            std::size_t allocationOffset;

            #ifdef KAYOU_DEBUG
            std::uint32_t magic;
            #endif
        };

        static std::size_t AlignForward(std::size_t address, std::size_t memAlignment);

        std::byte* m_start = nullptr;
        std::size_t m_offset = 0;
        std::size_t m_alignment = 0;
        std::size_t m_totalSize = 0;
        std::size_t m_usedSize = 0;
        std::size_t m_peakSize = 0;
    };
}
