#include "PoolAllocator.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>      // GCC std::uintptr_t
#include <cstdio>

#ifndef _WIN32
#include <cstdlib>      // GCC/CLang std::free & posix_memalign
#endif



namespace Kayou::Memory
{
    std::size_t PoolAllocator::AlignUp(const std::size_t size, const std::size_t memAlignment)
    {
        return (size + (memAlignment - 1)) & ~(memAlignment - 1);
    }


    std::size_t PoolAllocator::GetBlockIndex(const void* ptr) const
    {
        const std::uintptr_t ptrAddress = reinterpret_cast<std::uintptr_t>(ptr);
        const std::uintptr_t startAddress = reinterpret_cast<std::uintptr_t>(m_start);
        const std::size_t offset = ptrAddress - startAddress;

        return offset / m_blockStride;
    }


    PoolAllocator::PoolAllocator(const std::size_t blockCapacity, const std::size_t objectCount, const std::size_t memAlignment)
    {
        assert(blockCapacity > 0 && "PoolAllocator blockCapacity must be > 0");
        assert(objectCount > 0 && "PoolAllocator objectCount must be > 0");
        assert(std::has_single_bit(memAlignment) && "PoolAllocator memAlignment must be a power of 2");

        m_blockCapacity = blockCapacity;
        m_objectCount = objectCount;
        m_alignment = memAlignment;
        m_blockStates.resize(m_objectCount, BlockState::Free);

        // A memory block must hold the object and a free list pointer when available
        const std::size_t minBlockSize = std::max(blockCapacity, sizeof(FreeNode));
        m_blockStride = AlignUp(minBlockSize, memAlignment);
        m_totalSize = m_blockStride * m_objectCount;

        #ifdef _WIN32
        m_start = static_cast<std::byte*>(_aligned_malloc(m_totalSize, m_alignment));
        #else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, m_alignment, m_totalSize) != 0)
            ptr = nullptr;

        m_start = static_cast<std::byte*>(ptr);
        #endif

        assert(m_start && "PoolAllocator allocation failed");

        InitFreeList();
    }


    PoolAllocator::~PoolAllocator()
    {
        #ifdef _WIN32
        _aligned_free(m_start);
        #else
        std::free(m_start);
        #endif

        m_start = nullptr;
        m_freeList = nullptr;
        m_usedBlocks = 0;
        m_peakBlocks = 0;
    }


    void PoolAllocator::InitFreeList()
    {
        m_freeList = nullptr;

        for (std::size_t i = 0; i < m_objectCount; ++i)
        {
            FreeNode* node = reinterpret_cast<FreeNode*>(m_start + i * m_blockStride);
            node->m_next = m_freeList;
            m_freeList = node;
            m_blockStates[i] = BlockState::Free;
        }
        m_usedBlocks = 0;
    }


    void* PoolAllocator::Alloc(const std::size_t size, const std::size_t memAlignment)
    {
        // Fixed-size pool (can't allocate above the pool's capacity)
        if (size == 0 || size > m_blockCapacity)
            return nullptr;

        assert(std::has_single_bit(memAlignment) && "PoolAllocator alignment must be a power of 2");

        // Can't allocate if desired alignment is above the pool's alignment
        if (memAlignment > m_alignment)
            return nullptr;

        if (m_freeList == nullptr)
            return nullptr;

        FreeNode* node = m_freeList;
        m_freeList = node->m_next;

        const std::size_t blockIndex = GetBlockIndex(node);
        assert(blockIndex < m_objectCount && "PoolAllocator block index out of range");
        assert(m_blockStates[blockIndex] == BlockState::Free && "Allocating a block that is not marked free");

        m_blockStates[blockIndex] = BlockState::Used;

        ++m_usedBlocks;
        m_peakBlocks = std::max(m_peakBlocks, m_usedBlocks);

        return node;
    }


    void PoolAllocator::Free(void* ptr)
    {
        if (ptr == nullptr)
            return;

        const std::uintptr_t ptrAddress = reinterpret_cast<std::uintptr_t>(ptr);
        const std::uintptr_t startAddress = reinterpret_cast<std::uintptr_t>(m_start);
        const std::size_t endAddress = startAddress + m_totalSize;

        assert(ptrAddress >= startAddress && ptrAddress < endAddress && "Pointer does not belong to this pool");

        const std::size_t offset = ptrAddress - startAddress;
        assert(offset % m_blockStride == 0 && "Pointer is not aligned to the pool's block size");

        const std::size_t blockIndex = GetBlockIndex(ptr);
        assert(blockIndex < m_objectCount && "PoolAllocator block index out of range");
        assert(m_blockStates[blockIndex] == BlockState::Used && "Double free or freeing an already free block");

        m_blockStates[blockIndex] = BlockState::Free;

        FreeNode* node = static_cast<FreeNode*>(ptr);
        node->m_next = m_freeList;
        m_freeList = node;

        assert(m_usedBlocks > 0 && "PoolAllocator::Free is called too many times");
        --m_usedBlocks;
    }


    // Note that InitFreeList() does not reset m_peakBlocks
    void PoolAllocator::Reset()
    {
        InitFreeList();
    }


    void PoolAllocator::PrintUsage() const
    {
        printf("Pool Allocator: %zu / %zu blocks used (peak: %zu) | blockSize = %zu | total = %zu bytes\n",
            m_usedBlocks, m_objectCount, m_peakBlocks, m_blockStride, m_totalSize);
    }
}
