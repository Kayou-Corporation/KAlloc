#pragma once

#include <cstddef>
#include <cstdint>

#include "Utils/Utils.hpp"


namespace Kayou::Memory
{
    class FreeListAllocator
    {
    public:
        explicit FreeListAllocator(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        ~FreeListAllocator();

        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        void Free(void* ptr);
        void Reset();
        void PrintUsage() const;

        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetUsedSize() const { return m_usedSize; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetPeakSize() const { return m_peakSize; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetTotalSize() const { return m_totalSize; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetAlignment() const { return m_alignment; }

        FreeListAllocator(const FreeListAllocator&) = delete;
        FreeListAllocator(FreeListAllocator&&) = delete;
        FreeListAllocator& operator=(const FreeListAllocator&) = delete;
        FreeListAllocator& operator=(FreeListAllocator&&) = delete;


    private:
        struct FreeBlock
        {
            std::size_t size = 0;
            FreeBlock* next = nullptr;
        };

        struct AllocationHeader
        {
            std::size_t blockSize = 0;
            std::size_t adjustment = 0;

            #ifdef KAYOU_DEBUG
            std::uint32_t magic = 0;
            #endif
        };

        #ifdef KAYOU_DEBUG
        inline static constexpr std::uint32_t kFreeListAllocationHeaderMagic = 0xFEE1DEADu;
        #endif

        void InsertFreeBlock(FreeBlock* block);
        void MergeAdjacentFreeBlocks();

        std::byte* m_start = nullptr;
        FreeBlock* m_freeList = nullptr;

        std::size_t m_alignment = 0;
        std::size_t m_totalSize = 0;
        std::size_t m_usedSize = 0;
        std::size_t m_peakSize = 0;
    };
}
