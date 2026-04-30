/// TLSF = Two-Level Segregated Fit Allocator
///
/// Instead of scanning a single list of free blocks 
/// This allocator stores free blocks in multiple size classes:
/// - FLI: First Level Index    -   Coarse size class, log2(size) complexity
/// - SLI: Second Level Index   -   Finer subdivision inside each FLI bucket
///
///
///



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

        std::uint32_t m_fliBitmap = 0;
        std::uint32_t m_sliBitmap[FLICount] = { };

        BlockHeader* m_freeLists[FLICount][SLICount] = { };

        void* m_memory = nullptr;
        std::size_t m_totalSize = 0;
        std::size_t m_alignment = 0;

        std::size_t m_usedSize = 0;
        std::size_t m_peakSize = 0;

        static std::size_t AlignUp(std::size_t size, std::size_t alignment);

        void Mapping(std::size_t size, std::size_t& fli, std::size_t& sli) const;

        void InsertBlock(BlockHeader* block);
        void RemoveBlock(BlockHeader* block);

        BlockHeader* FindFreeBlock(std::size_t size, std::size_t& fli, std::size_t& sli) const;

        void Mapping(std::size_t size, std::size_t& fli, std::size_t& sli) const;

        void InsertBlock(BlockHeader* block);
        void RemoveBlock(BlockHeader* block);

        BlockHeader* FindSuitableBlock(std::size_t size);

        void SplitBlock(BlockHeader* block, std::size_t size);
        void MergeBlock(BlockHeader* block);
    };
}
