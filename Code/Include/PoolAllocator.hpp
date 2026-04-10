#pragma once

#include <cstddef>
#include <vector>

#include "Utils/Utils.hpp"


namespace Kayou::Memory
{
    class PoolAllocator
    {
    public:
        PoolAllocator(std::size_t blockCapacity, std::size_t objectCount, std::size_t memAlignment = alignof(std::max_align_t));
        ~PoolAllocator();

        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        void Free(void* ptr);
        void Reset();
        void PrintUsage() const;

        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetRawBlockCapacity() const { return m_blockCapacity; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetBlockStride() const { return m_blockStride; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetObjectCount() const { return m_objectCount; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetAlignment() const { return m_alignment; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetUsedBlocks() const { return m_usedBlocks; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetFreeBlocks() const { return m_objectCount - m_usedBlocks; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetPeakBlocks() const { return m_peakBlocks; }
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetTotalSize() const { return m_totalSize; }

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator(PoolAllocator&&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;
        PoolAllocator& operator=(PoolAllocator&&) = delete;


    private:
        struct FreeNode
        {
            FreeNode* m_next = nullptr;
        };

        enum class BlockState : std::uint8_t
        {
            Free    = 0,
            Used    = 1
        };


        static size_t AlignUp(size_t size, size_t memAlignment);
        std::size_t GetBlockIndex(const void* ptr) const;
        void InitFreeList();

        std::vector<BlockState> m_blockStates {};

        std::byte* m_start = nullptr;
        FreeNode* m_freeList = nullptr;

        std::size_t m_blockCapacity = 0;        // Maximum raw size accepted per allocation (internal capacity)
        std::size_t m_blockStride = 0;          // Actual aligned size between blocks in memory
        std::size_t m_objectCount = 0;
        std::size_t m_alignment = 0;
        std::size_t m_totalSize = 0;

        std::size_t m_usedBlocks = 0;
        std::size_t m_peakBlocks = 0;
    };
}
