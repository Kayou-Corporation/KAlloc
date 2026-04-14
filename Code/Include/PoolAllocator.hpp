#pragma once

#include <cstddef>
#include <vector>

#include "Utils/Utils.hpp"


namespace Kayou::Memory
{
    class PoolAllocator
    {
    public:
        /// @brief Constructor used to register a new Linear Allocator
        /// @param blockCapacity The desired block's capacity
        /// @param objectCount The number of objects
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        PoolAllocator(std::size_t blockCapacity, std::size_t objectCount, std::size_t memAlignment = alignof(std::max_align_t));
        ~PoolAllocator();

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator(PoolAllocator&&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;
        PoolAllocator& operator=(PoolAllocator&&) = delete;

        /// @brief Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new allocation
        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));

        /// @brief Function used to empty the allocation
        /// @param ptr Pointer to allocation to free
        void Free(void* ptr);

        /// @brief Function used to reset the linear allocator
        /// because, by concept, a Linear Allocator will free the entire allocated block.
        /// This will replace any Free() function
        void Reset();

        /// @brief Function used to print the allocator's usage
        void PrintUsage() const;

        /// @return The raw capacity (size) of the block
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetRawBlockCapacity() const     { return m_blockCapacity; }

        /// @return The stride (offset) of the block
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetBlockStride() const          { return m_blockStride; }

        /// @return The object count inside the block
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetObjectCount() const          { return m_objectCount; }

        /// @return The memory alignment of the block
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetAlignment() const            { return m_alignment; }

        /// @return The number of used blocks
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetUsedBlocks() const           { return m_usedBlocks; }

        /// @return The number of free blocks
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetFreeBlocks() const           { return m_objectCount - m_usedBlocks; }

        /// @return The peak block usage
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetPeakBlocks() const           { return m_peakBlocks; }

        /// @return The total available size
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetTotalSize() const            { return m_totalSize; }


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

        /// @brief Helper function used to retrieve the block index of the given pointer
        /// @param ptr The selected pointer
        /// @return The index of the block matching the specified pointer
        std::size_t GetBlockIndex(const void* ptr) const;


        /// @brief Helper function used to initialize/reset the free list to a proper default state
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
