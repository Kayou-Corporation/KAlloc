#include "FreeListAllocator.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdio>
#include <cstdint>      // GCC std::uint32_t & std::uintptr_t

#include "Utils/MemoryUtils.h"

#ifndef _WIN32
    #include <cstdlib>
#else
    #include <malloc.h>
#endif



namespace Kayou::Memory
{
    FreeListAllocator::FreeListAllocator(const std::size_t size, const std::size_t memAlignment)
    {
        assert(size > 0 && "FreeListAllocator size must be > 0");
        assert(std::has_single_bit(memAlignment) && "FreeListAllocator memAlignment must be power of 2");

        const std::size_t alignedSize = Internal::AlignUp(size, memAlignment);

        #ifdef _WIN32
        m_start = static_cast<std::byte*>(_aligned_malloc(alignedSize, memAlignment));
        #else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, memAlignment, alignedSize) != 0)
            ptr = nullptr;

        m_start = static_cast<std::byte*>(ptr);
        #endif

        assert(m_start && "FreeListAllocator allocation failed!");

        m_alignment = memAlignment;
        m_totalSize = alignedSize;
        m_usedSize = 0;
        m_peakSize = 0;

        m_freeList = reinterpret_cast<FreeBlock*>(m_start);
        m_freeList->size = m_totalSize;
        m_freeList->next = nullptr;
    }


    FreeListAllocator::~FreeListAllocator()
    {
        #ifdef _WIN32
        _aligned_free(m_start);
        #else
        std::free(m_start);
        #endif

        m_start = nullptr;
        m_freeList = nullptr;
        m_alignment = 0;
        m_totalSize = 0;
        m_usedSize = 0;
        m_peakSize = 0;
    }


    void* FreeListAllocator::Alloc(const std::size_t size, const std::size_t memAlignment)
    {
        if (size == 0)
            return nullptr;

        assert(std::has_single_bit(memAlignment) && "FreeListAllocator memAlignment must be power of 2");

        if (memAlignment > m_alignment)
            return nullptr;

        FreeBlock* previous = nullptr;
        FreeBlock* current = m_freeList;

        while (current != nullptr)
        {
            const std::uintptr_t blockStart = reinterpret_cast<std::uintptr_t>(current);
            const std::uintptr_t afterHeader = blockStart + sizeof(AllocationHeader);

            const std::uintptr_t userAddress = Internal::AlignForward(afterHeader, memAlignment);
            const std::size_t adjustment = static_cast<std::size_t>(userAddress - blockStart);

            std::size_t totalSize = adjustment + size;

            // Not enough space in this block
            if (current->size < totalSize)
            {
                previous = current;
                current = current->next;

                continue;
            }

            // Split block if enough space is available
            const std::size_t remainingSize = current->size - totalSize;
            if (remainingSize >= sizeof(FreeBlock))
            {
                FreeBlock* newBlock = reinterpret_cast<FreeBlock*>(blockStart + totalSize);
                newBlock->size = remainingSize;
                newBlock->next = current->next;

                if (previous)
                    previous->next = newBlock;
                else
                    m_freeList = newBlock;
            }
            else
            {
                // Consume the entire block
                totalSize = current->size;

                if (previous)
                    previous->next = current->next;
                else
                    m_freeList = current->next;
            }

            // Write allocation header
            AllocationHeader* header = reinterpret_cast<AllocationHeader*>(userAddress - sizeof(AllocationHeader));
            header->blockSize = totalSize;
            header->adjustment = adjustment;

            #ifdef KAYOU_DEBUG
            header->magic = kFreeListAllocationHeaderMagic;
            #endif

            // Update stats
            m_usedSize += totalSize;
            m_peakSize = std::max(m_peakSize, m_usedSize);

            return reinterpret_cast<void*>(userAddress);
        }

        // No matching block found
        return nullptr;
    }


    void FreeListAllocator::Free(void* ptr)
    {
        if (ptr == nullptr)
            return;

        const std::uintptr_t userAddress = reinterpret_cast<std::uintptr_t>(ptr);
        const std::uintptr_t startAddress = reinterpret_cast<std::uintptr_t>(m_start);
        [[maybe_unused]] const std::uintptr_t endAddress = startAddress + m_totalSize;

        assert(userAddress >= startAddress && userAddress < endAddress && "Pointer does not belong to this free list allocator");

        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(userAddress - sizeof(AllocationHeader));

        #ifdef KAYOU_DEBUG
        assert(header->magic == kFreeListAllocationHeaderMagic && "Invalid or corrupted FreeList allocation header");
        header->magic = 0;
        #endif

        const std::uintptr_t blockStart = userAddress - header->adjustment;
        const std::size_t blockSize = header->blockSize;

        assert(m_usedSize >= blockSize && "FreeListAllocator::Free underflow");
        m_usedSize -= blockSize;

        FreeBlock* block = reinterpret_cast<FreeBlock*>(blockStart);
        block->size = blockSize;
        block->next = nullptr;

        InsertFreeBlock(block);
    }


    void FreeListAllocator::Reset()
    {
        m_usedSize = 0;

        m_freeList = reinterpret_cast<FreeBlock*>(m_start);
        m_freeList->size = m_totalSize;
        m_freeList->next = nullptr;
    }


    void FreeListAllocator::PrintUsage() const
    {
        printf("FreeList Allocator: %zu bytes used (peak = %zu) / %zu total\n", m_usedSize, m_peakSize, m_totalSize);
    }


    void FreeListAllocator::InsertFreeBlock(FreeBlock* block)
    {
        if (m_freeList == nullptr)
        {
            m_freeList = block;
            return;
        }

        FreeBlock* previous = nullptr;
        FreeBlock* current = m_freeList;

        const std::uintptr_t blockAddress = reinterpret_cast<std::uintptr_t>(block);
        while (current != nullptr && reinterpret_cast<std::uintptr_t>(current) < blockAddress)
        {
            previous = current;
            current = current->next;
        }

        block->next = current;
        if (previous != nullptr)
            previous->next = block;
        else
            m_freeList = block;

        // Try merge with previous
        if (previous != nullptr)
        {
            const std::uintptr_t previousEnd =
                reinterpret_cast<std::uintptr_t>(previous) + previous->size;

            if (previousEnd == reinterpret_cast<std::uintptr_t>(block))
            {
                previous->size += block->size;
                previous->next = block->next;
                block = previous;
            }
        }

        // Try merge with next
        if (block->next != nullptr)
        {
            const std::uintptr_t blockEnd =
                reinterpret_cast<std::uintptr_t>(block) + block->size;

            if (blockEnd == reinterpret_cast<std::uintptr_t>(block->next))
            {
                block->size += block->next->size;
                block->next = block->next->next;
            }
        }
    }


    void FreeListAllocator::MergeAdjacentFreeBlocks()
    {
        FreeBlock* current = m_freeList;

        while (current != nullptr && current->next != nullptr)
        {
            const std::uintptr_t currentEnd = reinterpret_cast<std::uintptr_t>(current) + current->size;
            const std::uintptr_t nextAddress = reinterpret_cast<std::uintptr_t>(current->next);

            if (currentEnd == nextAddress)
            {
                current->size += current->next->size;
                current->next = current->next->next;
            }
            else
                current = current->next;
        }
    }
}
