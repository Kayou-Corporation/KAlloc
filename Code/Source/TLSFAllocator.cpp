#include "TLSFAllocator.hpp"

#include <algorithm>
#include <assert.h>
#include <bit>



namespace Kayou::Memory
{
    void TLSFAllocator::Mapping(const std::size_t size, std::size_t& fli, std::size_t& sli) const
    {
        const std::size_t adjustedSize = std::max(size, minBlockSize);
        const std::size_t highestBit = std::bit_width(adjustedSize) - 1;

        if (highestBit < 4)
        {
            fli = 0;
            sli = adjustedSize / (minBlockSize / SLICount);

            if (sli >= SLICount)
                sli = SLICount - 1;

            return;
        }

        fli = highestBit - 4;

        const std::size_t base = std::size_t{ 1 } << highestBit;
        const std::size_t offset = adjustedSize - base;
        const std::size_t sliceSize = base / SLICount;

        sli = offset / sliceSize;

        if (fli >= FLICount)
            fli = FLICount - 1;

        if (sli >= SLICount)
            sli = SLICount - 1;
    }


    void TLSFAllocator::InsertBlock(BlockHeader* block)
    {
        assert(block != nullptr);

        std::size_t fli = 0;
        std::size_t sli = 0;

        Mapping(block->size, fli, sli);

        // Mark block as free
        block->isFree = true;

        // Insert at head of list
        block->prev = nullptr;
        block->next = m_freeLists[fli][sli];

        if (block->next)
            block->next->prev = block;

        m_freeLists[fli][sli] = block;

        // Update bitmaps
        m_sliBitmaps[fli] |= (1u << sli);
        m_fliBitmap |= (1u << fli);
    }


    void TLSFAllocator::RemoveBlock(BlockHeader* block)
    {
        assert(block != nullptr);

        std::size_t fli = 0;
        std::size_t sli = 0;

        Mapping(block->size, fli, sli);

        #ifdef KAYOU_DEBUG
        assert(fli < kFLICount);
        assert(sli < kSLICount);
        assert(block->isFree && "TLSFAllocator::RemoveBlock expected a free block");
        #endif

        if (block->prev != nullptr)
            block->prev->next = block->next;
        else
            m_freeLists[fli][sli] = block->next;

        if (block->next != nullptr)
            block->next->prev = block->prev;

        block->prev = nullptr;
        block->next = nullptr;
        block->isFree = false;

        if (m_freeLists[fli][sli] == nullptr)
        {
            m_sliBitmaps[fli] &= ~(1u << sli);

            if (m_sliBitmaps[fli] == 0)
                m_fliBitmap &= ~(1u << fli);
        }
    }


    TLSFAllocator::BlockHeader* TLSFAllocator::FindSuitableBlock(const std::size_t size) const
    {
        std::size_t fli = 0;
        std::size_t sli = 0;

        Mapping(size, fli, sli);

        #ifdef KAYOU_DEBUG
        assert(fli < kFLICount);
        assert(sli < kSLICount);
        #endif

        // Search inside the current FLI, starting from the requested SLI
        const std::uint32_t sliMask = m_sliBitmaps[fli] & (~0u << sli);
        if (sliMask != 0)
        {
            const std::size_t foundSli = static_cast<std::size_t>(std::countr_zero(sliMask));
            return m_freeLists[fli][foundSli];
        }

        uint32_t fliMask = m_fliBitmap & (~0u << (fli + 1));
        if (fliMask == 0)
            return nullptr;

        const std::size_t foundFli = static_cast<std::size_t>(std::countr_zero(fliMask));
        const std::uint32_t foundSliMask = m_sliBitmaps[foundFli];

        #ifdef KAYOU_DEBUG
        assert(foundSliMask != 0);
        #endif

        const std::size_t foundSli = static_cast<std::size_t>(std::countr_zero(foundSliMask));
        return m_freeLists[foundFli][foundSli];
    }


    void TLSFAllocator::SplitBlock(BlockHeader* block, std::size_t size)
    {
        assert(block != nullptr);
        assert(block->size >= size);

        const std::size_t remainingSize = block->size - size;
        if (remainingSize < sizeof(BlockHeader) + minBlockSize)
            return;

        BlockHeader* newBlock = reinterpret_cast<BlockHeader*>(reinterpret_cast<std::byte*>(block) + size);
        newBlock->size = remainingSize;
        newBlock->prev = nullptr;
        newBlock->next = nullptr;
        newBlock->isFree = true;

        block->size = size;

        InsertBlock(newBlock);
    }
}
