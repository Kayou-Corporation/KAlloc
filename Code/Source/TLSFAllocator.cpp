#include "TLSFAllocator.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "Utils/MemoryUtils.hpp"

#ifdef _WIN32
#include <malloc.h>
#endif



namespace Kayou::Memory
{
    TLSFAllocator::TLSFAllocator(std::size_t size, std::size_t memAlignment)
    {
        assert(size > 0 && "TLSFAllocator size must be > 0");
        assert(std::has_single_bit(memAlignment) && "TLSFAllocator memAlignment must be power of 2");

        const std::size_t alignedSize = Internal::AlignUp(size, memAlignment);

        #ifdef _WIN32
        m_memory = _aligned_malloc(alignedSize, memAlignment);
        #else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, memAlignment, alignedSize) != 0)
            ptr = nullptr;

        m_memory = ptr;
        #endif

        assert(m_memory != nullptr && "TLSFAllocator allocation failed");

        m_totalSize = alignedSize;
        m_alignment = memAlignment;
        m_usedSize = 0;
        m_peakSize = 0;

        Reset();
    }


    TLSFAllocator::~TLSFAllocator()
    {
        #ifdef _WIN32
        _aligned_free(m_memory);
        #else
        std::free(m_memory);
        #endif

        m_memory = nullptr;
        m_totalSize = 0;
        m_alignment = 0;
        m_usedSize = 0;
        m_peakSize = 0;
        m_fliBitmap = 0;

        for (std::size_t fli = 0; fli < FLICount; ++fli)
        {
            m_sliBitmaps[fli] = 0;

            for (std::size_t sli = 0; sli < SLICount; ++sli)
                m_freeLists[fli][sli] = nullptr;
        }
    }


    void* TLSFAllocator::Alloc(const std::size_t size, const std::size_t memAlignment)
    {
        if (size == 0)
            return nullptr;

        assert(std::has_single_bit(memAlignment) && "TLSFAllocator memAlignment must be power of 2");

        constexpr std::size_t blockAlignment = alignof(BlockHeader);
        constexpr std::size_t headerAlignment = alignof(BlockHeader);
        constexpr std::size_t internalAlignment = std::max(blockAlignment, headerAlignment);
        const std::size_t effectiveAlignment = std::max(memAlignment, headerAlignment);
        const std::size_t requiredSize = sizeof(BlockHeader) + sizeof(AllocationHeader) + effectiveAlignment - 1 + size;

        // Align size to remain coherent with TLSF
        const std::size_t alignedSize = Internal::AlignUp(requiredSize, internalAlignment);

        // Find a block
        BlockHeader* block = FindSuitableBlock(alignedSize);
        if (block == nullptr)
            return nullptr;

        // Remove the block from the free lists
        RemoveBlock(block);

        // Split if necessary
        SplitBlock(block, alignedSize);

        block->isFree = false;

        // Get the user address
        const std::uintptr_t blockAddress = reinterpret_cast<std::uintptr_t>(block);
        const std::uintptr_t afterHeaders = blockAddress + sizeof(BlockHeader) + sizeof(AllocationHeader);
        const std::uintptr_t userAddress = Internal::AlignForward(afterHeaders, effectiveAlignment);

        AllocationHeader* allocationHeader = reinterpret_cast<AllocationHeader*>(userAddress - sizeof(AllocationHeader));

        allocationHeader->block = block;

        #ifdef KAYOU_DEBUG
        assert(reinterpret_cast<std::uintptr_t>(allocationHeader) % alignof(AllocationHeader) == 0);
        assert(userAddress % memAlignment == 0);
        #endif

        m_usedSize += block->size;
        m_peakSize = std::max(m_peakSize, m_usedSize);

        return reinterpret_cast<void*>(userAddress);
    }


    void TLSFAllocator::Free(void* ptr)
    {
        if (ptr == nullptr)
            return;

        const std::uintptr_t userAddress = reinterpret_cast<std::uintptr_t>(ptr);
        const std::uintptr_t memoryStart = reinterpret_cast<std::uintptr_t>(m_memory);
        const std::uintptr_t memoryEnd = memoryStart + m_totalSize;

        assert(userAddress >= memoryStart && userAddress < memoryEnd && "Pointer does not belong to this TLSFAllocator");

        const AllocationHeader* allocationHeader = reinterpret_cast<AllocationHeader*>(reinterpret_cast<std::byte*>(ptr) - sizeof(AllocationHeader));
        BlockHeader* block = allocationHeader->block
        ;
        #ifdef KAYOU_DEBUG
        assert(block != nullptr && "TLSFAllocator::Free invalid allocation header");
        assert(!block->isFree && "TLSFAllocator::Free called twice");
        #endif

        block->isFree = true;

        assert(m_usedSize >= block->size && "TLSFAllocator::Free underflow");
        m_usedSize -= block->size;

        block = MergeBlock(block);
        InsertBlock(block);
    }


    void TLSFAllocator::Reset()
    {
        m_usedSize = 0;
        m_fliBitmap = 0;

        for (std::size_t fli = 0; fli < FLICount; ++fli)
        {
            m_sliBitmaps[fli] = 0;

            for (std::size_t sli = 0; sli < SLICount; ++sli)
                m_freeLists[fli][sli] = nullptr;
        }

        BlockHeader* initialBlock = static_cast<BlockHeader*>(m_memory);
        initialBlock->size = m_totalSize;
        initialBlock->prev = nullptr;
        initialBlock->next = nullptr;
        initialBlock->isFree = true;

        InsertBlock(initialBlock);
    }


    void TLSFAllocator::PrintUsage() const
    {
        printf("TLSF Allocator: %zu used / %zu total | peak = %zu\n", m_usedSize, m_totalSize, m_peakSize);
        printf("Free blocks: %zu | Largest free: %zu | Fragmentation: %.2f%%\n", GetFreeBlockCount(), GetLargestFreeBlockSize(), GetFragmentationRatio() * 100.0);
    }


    std::size_t TLSFAllocator::GetFreeBlockCount() const
    {
        std::size_t count = 0;

        for (std::size_t fli = 0; fli < FLICount; ++fli)
        {
            for (std::size_t sli = 0; sli < SLICount; ++sli)
            {
                const BlockHeader* current = m_freeLists[fli][sli];

                while (current != nullptr)
                {
                    ++count;
                    current = current->next;
                }
            }
        }
        return count;
    }


    std::size_t TLSFAllocator::GetLargestFreeBlockSize() const
    {
        std::size_t largest = 0;

        for (std::size_t fli = 0; fli < FLICount; ++fli)
        {
            for (std::size_t sli = 0; sli < SLICount; ++sli)
            {
                const BlockHeader* current = m_freeLists[fli][sli];

                while (current != nullptr)
                {
                    largest = std::max(largest, current->size);
                    current = current->next;
                }
            }
        }
        return largest;
    }


    std::size_t TLSFAllocator::GetTotalFreeSize() const
    {
        std::size_t total = 0;

        for (std::size_t fli = 0; fli < FLICount; ++fli)
        {
            for (std::size_t sli = 0; sli < SLICount; ++sli)
            {
                const BlockHeader* current = m_freeLists[fli][sli];

                while (current != nullptr)
                {
                    total += current->size;
                    current = current->next;
                }
            }
        }
        return total;
    }


    double TLSFAllocator::GetFragmentationRatio() const
    {
        const std::size_t totalFree = GetTotalFreeSize();
        if (totalFree == 0)
            return 0.0;

        const std::size_t largestFree = GetLargestFreeBlockSize();

        return 1.0 - (static_cast<double>(largestFree) / static_cast<double>(totalFree));
    }


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
        assert(fli < FLICount);
        assert(sli < SLICount);
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
        assert(fli < FLICount);
        assert(sli < SLICount);
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


    TLSFAllocator::BlockHeader* TLSFAllocator::MergeBlock(BlockHeader* block)
    {
        assert(block != nullptr);

        BlockHeader* nextBlock = reinterpret_cast<BlockHeader*>(reinterpret_cast<std::byte*>(block) + block->size);

        const std::byte* memoryEnd = static_cast<const std::byte*>(m_memory) + m_totalSize;
        if (reinterpret_cast<const std::byte*>(nextBlock) >= memoryEnd)
            return block;

        if (!nextBlock->isFree)
            return block;

        RemoveBlock(nextBlock);

        block->size += nextBlock->size;

        return block;
    }
}
