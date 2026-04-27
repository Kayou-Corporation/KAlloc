#pragma once

#include <cstddef>
#include <cstdint>

#include "Utils/Utils.hpp"



namespace Kayou::Memory
{
    class FreeListAllocator
    {
    public:

        /// @brief Constructor used to register a new Free List Allocator
        /// @param size The total size of the allocator
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        explicit FreeListAllocator(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        ~FreeListAllocator();

        FreeListAllocator(const FreeListAllocator&) = delete;
        FreeListAllocator(FreeListAllocator&&) = delete;
        FreeListAllocator& operator=(const FreeListAllocator&) = delete;
        FreeListAllocator& operator=(FreeListAllocator&&) = delete;

        /// @brief Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new allocation
        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));

        /// @brief Function used to free the selected pointer
        /// @param ptr The pointer that should be freed
        void Free(void* ptr);

        /// @brief Function used to reset the free list allocator
        void Reset();

        /// @brief Function used to print the allocator's usage
        void PrintUsage() const;

        /// @return The size of the memory currently in use, in bytes
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetUsedSize() const            { return m_usedSize; }

        /// @return The peak used size reached by the allocator
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetPeakSize() const            { return m_peakSize; }

        /// @return The total size of the allocator (free and used), in bytes
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetTotalSize() const           { return m_totalSize; }

        /// @return The current memory alignment of the allocator
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetAlignment() const           { return m_alignment; }


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