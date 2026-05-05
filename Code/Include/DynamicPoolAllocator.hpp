#pragma once

#include <cstddef>
#include <vector>

#include "Utils/Utils.hpp"



namespace Kayou::Memory
{
    class DynamicPoolAllocator
    {
    public:
        /// @brief Constructor used to register a new Dynamic PoolAllocator
        /// @param blockCapacity The capacity of each block
        /// @param blocksPerPage The number of blocks per page
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        DynamicPoolAllocator(std::size_t blockCapacity, std::size_t blocksPerPage, std::size_t memAlignment = alignof(std::max_align_t));
        ~DynamicPoolAllocator();

        DynamicPoolAllocator(const DynamicPoolAllocator&) = delete;
        DynamicPoolAllocator(DynamicPoolAllocator&&) = delete;
        DynamicPoolAllocator& operator=(const DynamicPoolAllocator&) = delete;
        DynamicPoolAllocator& operator=(DynamicPoolAllocator&&) = delete;

        /// @brief Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new allocation
        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));

        /// @brief Function used to empty the allocation
        /// @param ptr Pointer to allocation to free
        void Free(void* ptr);

        /// @brief Resets the allocator to its initial state by rebuilding the free list
        ///        All blocks are marked as free, and the internal allocation state is fully reinitialized
        ///        Does not release memory back to the system
        void Reset();

        /// @brief Function used to print the allocator's usage
        void PrintUsage() const;

        /// @return The raw capacity (size) of the block
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetRawBlockCapacity() const         { return m_blockCapacity; }

        /// @return The stride (offset) of the block
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetBlockStride() const              { return m_blockStride; }

        /// @return The object count inside the block
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetBlocksPerPage() const            { return m_blocksPerPage; }

        /// @return The object count inside the block
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetPageCount() const                { return m_pages.size(); }

        /// @return The memory alignment of the block
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetAlignment() const                { return m_alignment; }

        /// @return The total size of the allocator (free and used), in bytes
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE size_t GetTotalSize() const                { return m_totalSize; }

        /// @return The number of used blocks
        KAYOU_NO_DISCARD size_t GetUsedBlocks() const;

        /// @return The number of free blocks
        KAYOU_NO_DISCARD size_t GetFreeBlocks() const;

        /// @return The peak block usage
        KAYOU_NO_DISCARD size_t GetPeakBlocks() const;

        /// @return The total number of blocks (free and used) across all pages
        KAYOU_NO_DISCARD std::size_t GetTotalBlockCount() const;




    private:
        struct FreeNode
        {
            FreeNode* next = nullptr;
        };

        enum class BlockState : std::uint8_t
        {
            Free    = 0,
            Used    = 1,
            COUNT
        };

        struct Page
        {
            std::byte* start = nullptr;
            FreeNode* freeList = nullptr;

            std::vector<BlockState> blockStates { };

            std::size_t usedBlocks = 0;
            std::size_t peakBlocks = 0;
        };

        /// @brief Helper function used to create a new page when required
        Page& CreatePage();

        /// @brief Helper function used to initialize/reset the free list of a page to a proper default state
        /// @param page The page to initialize
        void InitPageFreeList(Page& page);

        /// @brief Helper function used to find the page owning the given pointer
        /// @param ptr The selected pointer
        /// @return A pointer to the page owning the specified pointer, or nullptr if not found
        Page* FindOwningPage(const void* ptr);

        /// @brief Const version of FindOwningPage
        /// @param ptr The selected pointer
        const Page* FindOwningPage(const void* ptr) const;

        /// @brief Helper function used to retrieve the block index of the given pointer
        /// @param ptr The selected pointer
        /// @return The index of the block matching the specified pointer
        std::size_t GetBlockIndex(const Page& page, const void* ptr) const;

        std::vector<Page> m_pages { };

        std::size_t m_blockCapacity = 0;        // Maximum raw size accepted per allocation (internal capacity)
        std::size_t m_blockStride = 0;          // Actual aligned size between blocks in memory
        std::size_t m_blocksPerPage = 0;
        std::size_t m_alignment = 0;
        std::size_t m_totalSize = 0;
    };
}
