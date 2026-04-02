#include "LinearAllocator.hpp"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>


namespace Kayou
{
    ///
    /// @param ptrAddress The pointer's address
    /// @param memAlignment The desired memory alignment (always a multiple of 2)
    /// @return The new address for the pointer
    static std::size_t AlignForward(std::size_t ptrAddress, const std::size_t memAlignment)
    {
        const std::size_t offset = ptrAddress % memAlignment;
        if (offset != 0)
            ptrAddress += (memAlignment - offset);

        return ptrAddress;
    }


    LinearAllocator::LinearAllocator(std::size_t size)
    {
        m_start = static_cast<std::byte*>(std::malloc(size));
        p_totalSize = size;
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

        if (m_offset + padding + size > p_totalSize)
            return nullptr;

        m_offset += padding;

        void* result = m_start + m_offset;
        m_offset += size;

        TrackAlloc(size);

        return result;
    }


    void LinearAllocator::Reset()
    {
        m_offset = 0;
        p_usedSize = 0;
    }


    void LinearAllocator::PrintUsage() const
    {
        std::printf("Linear Allocator: %zu / %zu bytes used!\n", p_usedSize, p_totalSize);
    }
}
