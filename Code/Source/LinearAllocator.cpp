#include "LinearAllocator.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>      // GCC std::uint32_t & std::uintptr_t
#include <cstdio>
#include <stdexcept>

#include "Utils/MemoryUtils.hpp"

#ifdef _WIN32
    #include <malloc.h>     // MSVC _aligned_malloc & _aligned_free
#else
    #include <cstdlib>      // GCC/CLang std::free & posix_memalign
#endif



namespace Kayou::Memory
{

    LinearAllocator::LinearAllocator(std::size_t size, std::size_t memAlignment)
    {
        assert(size > 0 && "LinearAllocator size must be > 0");
        assert(std::has_single_bit(memAlignment) && "LinearAllocator memAlignment must be power of 2!");
        size = Internal::AlignUp(size, memAlignment);

        #ifdef _WIN32
        m_start = static_cast<std::byte*>(_aligned_malloc(size, memAlignment));
        #else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, memAlignment, size) != 0)
            ptr = nullptr;

        m_start = static_cast<std::byte*>(ptr);
        #endif

        assert(m_start && "LinearAllocator allocation failed!");

        m_totalSize = size;
        m_offset = 0;
    }


    LinearAllocator::~LinearAllocator()
    {
        #ifdef _WIN32
        _aligned_free(m_start);
        #else
        std::free(m_start);
        #endif

        m_start = nullptr;
        m_offset = 0;
        m_usedSize = 0;
        m_peakSize = 0;
        m_totalSize = 0;
    }


    void* LinearAllocator::Alloc(const std::size_t size, const std::size_t memAlignment)
    {
        if (size == 0)
            return nullptr;

        assert(std::has_single_bit(memAlignment) && "LinearAllocator memAlignment must be power of 2!");

        const std::uintptr_t currentAddress = reinterpret_cast<std::uintptr_t>(m_start) + m_offset;
        const std::uintptr_t alignedAddress = (currentAddress + (memAlignment - 1)) & ~(memAlignment - 1);
        const std::size_t padding = alignedAddress - currentAddress;

        if (m_offset + padding + size > m_totalSize)
            return nullptr;

        m_offset += padding;
        void* result = m_start + m_offset;
        m_offset += size;

        const std::size_t actualSize = size + padding;
        m_usedSize += actualSize;
        m_peakSize = std::max(m_peakSize, m_usedSize);

        return result;
    }


    void LinearAllocator::Reset()
    {
        m_offset = 0;
        m_usedSize = 0;
    }


    void LinearAllocator::PrintUsage() const
    {
        printf("Linear Allocator: %zu bytes used (offset = %zu, peak = %zu) / %zu total\n", m_usedSize, m_offset, m_peakSize, m_totalSize);
    }
}