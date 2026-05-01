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
        explicit TLSFAllocator(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        ~TLSFAllocator();

        TLSFAllocator(const TLSFAllocator&) = delete;
        TLSFAllocator(TLSFAllocator&&) = delete;
        TLSFAllocator& operator=(const TLSFAllocator&) = delete;
        TLSFAllocator& operator=(TLSFAllocator&&) = delete;

        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        void Free(void* ptr);
        void Reset();
        void PrintUsage() const;


        KAYOU_NO_DISCARD std::size_t GetUsedSize() const;
        KAYOU_NO_DISCARD std::size_t GetPeakSize() const;
        KAYOU_NO_DISCARD std::size_t GetTotalSize() const;
        KAYOU_NO_DISCARD std::size_t GetAlignment() const;


    private:
        /// @brief Header placed before each block
        ///
        /// For free blocks, prev/next are used to link the block into a TLSF free list
        /// For used blocks, prev/next are not meaningful
        ///
        /// `size` stores the full block size (header + data)
        struct BlockHeader
        {
            std::size_t size = 0;
            BlockHeader* prev = nullptr;
            BlockHeader* next = nullptr;
            bool isFree = true;
        };

        static const std::size_t minBlockSize = 16;
        static const std::size_t FLICount = 32;
        static const std::size_t SLICount = 16;

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

        static std::size_t AlignUp(std::size_t size, std::size_t alignment);

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

        BlockHeader* FindFreeBlock(std::size_t size, std::size_t& fli, std::size_t& sli) const;

        /// @brief Finds a free block large enough for the requested size
        ///        Instead of scanning all free blocks, TLSF checks the bitmaps
        ///        to quickly find a bucket that contains a suitable block
        ///
        /// @returns nullptr if no block is availabnle
        BlockHeader* FindSuitableBlock(std::size_t size) const;

        /// @brief Splits a large block into two separate blocks
        ///        The first part keeps the requested size
        ///        The remaining part becomes a new free block and is inserted into TLSF
        void SplitBlock(BlockHeader* block, std::size_t size);
        void MergeBlock(BlockHeader* block);
    };
}
