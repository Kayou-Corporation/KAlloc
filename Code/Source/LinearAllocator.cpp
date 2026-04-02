#include "LinearAllocator.hpp"

#include <cassert>
#include <stdexcept>

#ifndef _WIN32
    #include <cstdlib>
#endif



namespace Kayou
{
    ///
    /// @param ptrAddress The pointer's address
    /// @param memAlignment The desired memory alignment (always a multiple of 2)
    /// @return The new address for the pointer
    std::size_t LinearAllocator::AlignForward(const std::size_t ptrAddress, const std::size_t memAlignment)
    {
        return (ptrAddress + (memAlignment - 1)) & ~ (memAlignment - 1);
    }


    LinearAllocator::LinearAllocator(std::size_t size, std::size_t memAlignment)
    {
        assert(std::has_single_bit(memAlignment) && "memAlignment must be a power of 2!");

        size = (size + memAlignment - 1) & ~(memAlignment - 1);

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
        std::free(m_start);
        m_start = nullptr;
    }


    void* LinearAllocator::Alloc(std::size_t size, std::size_t memAlignment)
    {
        const std::size_t currentAddress = reinterpret_cast<std::size_t>(m_start) + m_offset;
        const std::size_t alignedAddress = AlignForward(currentAddress, memAlignment);
        const std::size_t padding = alignedAddress - currentAddress;

        if (m_offset + padding + size > m_totalSize)
            return nullptr;

        m_offset += padding;

        void* result = m_start + m_offset;
        m_offset += size;

        TrackAlloc(size + padding); // Track the real size, including padding for better accuracy

        return result;
    }


    void LinearAllocator::Reset()
    {
        m_offset = 0;
        m_usedSize = 0;
    }


    void LinearAllocator::PrintUsage() const
    {
        std::printf("Linear Allocator: %zu / %zu bytes used!\n", m_usedSize, m_totalSize);
    }


    void LinearAllocator::TrackAlloc(std::size_t size)
    {
        m_usedSize += size;
        m_peakSize = std::max(m_peakSize, m_usedSize);
    }
}
