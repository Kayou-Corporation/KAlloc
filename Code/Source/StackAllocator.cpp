#include "StackAllocator.hpp"

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
    StackAllocator::StackAllocator(const std::size_t size, const std::size_t memAlignment)
    {
        assert(size > 0 && "StackAllocator size must be > 0");
        assert(std::has_single_bit(memAlignment) && "StackAllocator memAlignment must be power of 2");

        const std::size_t alignedSize = Internal::AlignUp(size, memAlignment);

        #ifdef _WIN32
        m_start = static_cast<std::byte*>(_aligned_malloc(alignedSize, memAlignment));
        #else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, memAlignment, alignedSize) != 0)
            ptr = nullptr;

        m_start = static_cast<std::byte*>(ptr);
        #endif

        assert(m_start && "StackAllocator allocation failed");

        m_totalSize = alignedSize;
        m_alignment = memAlignment;
        m_offset = 0;
        m_usedSize = 0;
        m_peakSize = 0;
    }


    StackAllocator::~StackAllocator()
    {
        #ifdef _WIN32
        _aligned_free(m_start);
        #else
        std::free(m_start);
        #endif

        m_start = nullptr;
        m_offset = 0;
        m_alignment = 0;
        m_totalSize = 0;
        m_usedSize = 0;
        m_peakSize = 0;
    }


    void* StackAllocator::Alloc(const std::size_t size, const std::size_t memAlignment)
    {
        if (size == 0)
            return nullptr;

        assert(std::has_single_bit(memAlignment) && "StackAllocator alignment must be power of 2");

        // Requested alignment not supported by this allocator
        if (memAlignment > m_alignment)
            return nullptr;

        const std::uintptr_t currentAddress = reinterpret_cast<std::uintptr_t>(m_start) + m_offset;
        const std::uintptr_t afterHeader = currentAddress + sizeof(AllocationHeader);
        const std::uintptr_t userAddress = Internal::AlignForward(afterHeader, memAlignment);

        const std::size_t newOffset = static_cast<std::size_t>((userAddress - reinterpret_cast<std::uintptr_t>(m_start)) + size);
        if (newOffset > m_totalSize)
            return nullptr;

        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(userAddress - sizeof(AllocationHeader));
        header->previousOffset = m_offset;
        header->allocationOffset = newOffset;

        #ifdef KAYOU_DEBUG
        header->magic = kStackAllocationHeaderMagic;
        #endif

        m_offset = newOffset;
        m_usedSize = m_offset;
        m_peakSize = std::max(m_peakSize, m_usedSize);

        return reinterpret_cast<void*>(userAddress);
    }


    void StackAllocator::Free(void* ptr)
    {
        if (ptr == nullptr)
            return;

        const std::uintptr_t ptrAddress = reinterpret_cast<std::uintptr_t>(ptr);
        const std::uintptr_t startAddress = reinterpret_cast<std::uintptr_t>(m_start);
        [[maybe_unused]] const std::uintptr_t endAddress = startAddress + m_totalSize;

        assert(ptrAddress >= startAddress && ptrAddress < endAddress && "Pointer does not belong to this stack allocator");

        AllocationHeader* header = reinterpret_cast<AllocationHeader*>(ptrAddress - sizeof(AllocationHeader));

        #ifdef KAYOU_DEBUG
        assert(header->magic == kStackAllocationHeaderMagic && "Invalid or corrupted stack allocation header");
        assert(header->allocationOffset == m_offset && "StackAllocator::Free must follow LIFO order");
        header->magic = 0u;
        #endif


        m_offset = header->previousOffset;
        m_usedSize = m_offset;
    }


    void StackAllocator::Pop(void* ptr)
    {
        Free(ptr);
    }


    void StackAllocator::Reset()
    {
        m_offset = 0;
        m_usedSize = 0;
    }


    void StackAllocator::PrintUsage() const
    {
        printf("Stack Allocator: %zu bytes used (offset = %zu, peak = %zu) / %zu total\n", m_usedSize, m_offset, m_peakSize, m_totalSize);
    }

}
