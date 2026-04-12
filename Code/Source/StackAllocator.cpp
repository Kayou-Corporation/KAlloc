#include "StackAllocator.hpp"

#include <bit>
#include <cassert>


namespace Kayou::Memory
{
    std::size_t StackAllocator::AlignForward(std::size_t address, std::size_t memAlignment)
    {
        return (address + (memAlignment - 1)) & ~(memAlignment - 1);
    }


    StackAllocator::StackAllocator(const std::size_t size, const std::size_t memAlignment)
    {
        assert(size > 0 && "StackAllocator size must be > 0");
        assert(std::has_single_bit(memAlignment) && "StackAllocator memAlignment must be a power of 2");

        const std::size_t alignedSize = AlignForward(size, memAlignment);

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
        if (size <= 0)
            return nullptr;

        assert(std::has_single_bit(memAlignment) && "StackAllocator alignment must be a power of 2");

        // Can't allocate if desired alignment is above the pool's alignment
        if (memAlignment > m_alignment)
            return nullptr;

        std::uintptr_t currentAddress = reinterpret_cast<std::uintptr_t>(m_start) + m_offset;
    }


    void StackAllocator::Free(void* ptr)
    {
    }


    void StackAllocator::Reset()
    {
    }


    void StackAllocator::PrintUsage() const
    {
    }

}
