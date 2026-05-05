/// TLSF = Two-Level Segregated Fit Allocator
///
/// Instead of scanning a single list of free blocks,
/// TLSF organizes memory into buckets (categories) based on size
/// Each bucket is further split into small subcategories
/// - FLI: First Level Index    -   Groups blocks by size range (power of two)
/// - SLI: Second Level Index   -   Splits each FLI into smaller buckets for more precision
///
/// FLI 0   =   Small blocks        SLI 0   =   [64-71]
/// FLI 1   =   Medium blocks       SLI 2   =   [72-79]
/// FLI 2   =   Bigger blocks       SLI 3   =   [80-89]

#pragma once

#include <cstddef>

#include "Utils/Utils.hpp"


namespace Kayou::Memory
{
    class TLSFAllocator
    {
    public:
        /// @brief Constructor used to register a new TLSF Allocator
        /// @param size The total size of the allocator
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        explicit TLSFAllocator(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        ~TLSFAllocator();

        TLSFAllocator(const TLSFAllocator&) = delete;
        TLSFAllocator(TLSFAllocator&&) = delete;
        TLSFAllocator& operator=(const TLSFAllocator&) = delete;
        TLSFAllocator& operator=(TLSFAllocator&&) = delete;

        /// @brief Allocated memory using the TLSF algorithm
        /// @param size The size of the allocation
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        /// @return nullptr if no suitable block is found
        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        void Free(void* ptr);
        void Reset();
        void PrintUsage() const;

        /// @return The size of the memory currently in use, in bytes
        KAYOU_NO_DISCARD std::size_t GetUsedSize() const            { return m_usedSize; }

        /// @return The peak used size reached by the allocator
        KAYOU_NO_DISCARD std::size_t GetPeakSize() const            { return m_peakSize; }

        /// @return The total size of the allocator (free and used), in bytes
        KAYOU_NO_DISCARD std::size_t GetTotalSize() const           { return m_totalSize; }

        /// @return The current memory alignment of the allocator
        KAYOU_NO_DISCARD std::size_t GetAlignment() const           { return m_alignment; }

        /// @return The number of free blocks currently available in the allocator
        KAYOU_NO_DISCARD std::size_t GetFreeBlockCount() const;

        /// @return The size of the largest free block currently available in the allocator, in bytes
        KAYOU_NO_DISCARD std::size_t GetLargestFreeBlockSize() const;

        /// @return The total size of all free blocks currently available in the allocator, in bytes
        KAYOU_NO_DISCARD std::size_t GetTotalFreeSize() const;

        /// @return The fragmentation ratio of the allocator, calculated as (1 - (largest free block size / total free size))
        KAYOU_NO_DISCARD double GetFragmentationRatio() const;


    private:
        /// @brief Header placed before each block
        ///
        /// For free blocks, prev/next are used to link the block into a TLSF free list
        /// For used blocks, prev/next are not meaningful
        ///
        /// `size` stores the full block size (header and data)
        struct BlockHeader
        {
            std::size_t size = 0;

            BlockHeader* previousPhysical = nullptr; // Pointer to the previous physical block in memory (used for merging)

            BlockHeader* prev = nullptr;
            BlockHeader* next = nullptr;

            bool isFree = true;
        };

        struct AllocationHeader
        {
            BlockHeader* block = nullptr;
        };

        inline static const std::size_t minBlockSize = 16;
        inline static const std::size_t FLICount = 32;
        inline static const std::size_t SLICount = 16;

        /// @brief Main bitmap (which FLI buckets contain something)
        std::uint32_t m_fliBitmap = 0;

        /// @brief Sub-bitmaps (which SLI buckets contain something)
        std::uint32_t m_sliBitmaps[FLICount] = { };

        /// @brief Free lists sorted by size
        BlockHeader* m_freeLists[FLICount][SLICount] = { };

        void* m_memory = nullptr;
        std::size_t m_totalSize = 0;
        std::size_t m_alignment = 0;

        std::size_t m_usedSize = 0;
        std::size_t m_peakSize = 0;

        /// @brief Converts a size into a mapping (FLI, SLI)
        ///        This tells us in which bucket the data block
        ///        should be stored based on its size
        ///
        /// @param size Requested block size, including allocator metadata when relevant
        /// @param fli Output first-level index
        /// @param sli Output second-level index
        void Mapping(std::size_t size, std::size_t& fli, std::size_t& sli) const;

        void InsertBlock(BlockHeader* block);

        /// @brief Removes a free block from its TLSF bucket
        ///        The block must already be inside the free list matching its size
        ///        After removal; the bucket bitmaps are updated if the bucket becomes empty
        ///
        /// @param block The block to be removed
        void RemoveBlock(BlockHeader* block);

        /// @brief Finds a free block large enough for the requested size
        ///        Instead of scanning all free blocks, TLSF checks the bitmaps
        ///        to quickly find a bucket that contains a suitable block
        ///
        /// @return nullptr if no block is availabnle
        BlockHeader* FindSuitableBlock(std::size_t size) const;

        /// @brief Splits a large block into two separate blocks
        ///        The first part keeps the requested size
        ///        The remaining part becomes a new free block and is inserted into TLSF
        void SplitBlock(BlockHeader* block, std::size_t size);

        /// @brief Merges a block with the next block if it is free
        ///        This helps reduce fragmentation by combining adjacent blocks
        ///
        /// @return The merged block
        BlockHeader* MergeBlock(BlockHeader* block);
    };
}
