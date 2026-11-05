#include "ArenaAllocator.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <utility>

#include "Utils/MemoryUtils.hpp"

#ifdef _WIN32
    #include <malloc.h>
#endif



namespace Kayou::Memory
{
    ArenaAllocator::ArenaAllocator(const std::size_t pageSize, const std::size_t memAlignment)
    {
        assert(pageSize > 0 && "ArenaAllocator pageSize must be > 0");
        assert(std::has_single_bit(memAlignment) && "ArenaAllocator memAlignment must be power of 2");

        m_alignment = memAlignment;
        m_defaultPageSize = Internal::AlignUp(pageSize, m_alignment);

        CreatePage(m_defaultPageSize);
    }


    ArenaAllocator::~ArenaAllocator()
    {
        for (Page& page : m_pages)
        {
        #ifdef _WIN32
            _aligned_free(page.start);
        #else
            std::free(page.start);
        #endif

            page.start = nullptr;
            page.offset = 0;
            page.usedSize = 0;
            page.peakSize = 0;
            page.capacity = 0;
        }

        m_pages.clear();
        m_defaultPageSize = 0;
        m_alignment = 0;
    }


    void* ArenaAllocator::Alloc(const std::size_t size, const std::size_t memAlignment)
    {
        if (size == 0)
            return nullptr;

        assert(std::has_single_bit(memAlignment) && "ArenaAllocator memAlignment must be power of 2");

        if (memAlignment > m_alignment)
            return nullptr;

        Page* page = &GetCurrentPage();

        std::uintptr_t baseAddress = reinterpret_cast<std::uintptr_t>(page->start);
        std::uintptr_t currentAddress = baseAddress + page->offset;
        std::uintptr_t alignedAddress = Internal::AlignForward(currentAddress, memAlignment);

        std::size_t padding = static_cast<std::size_t>(alignedAddress - currentAddress);
        std::size_t requiredSize = padding + size;

        if (page->offset + requiredSize > page->capacity)
        {
            if (m_currentPageIndex + 1 < m_pages.size())
            {
                ++m_currentPageIndex;
                page = &GetCurrentPage();
            }
            else
            {
                page = &CreatePage(size + memAlignment);
            }

            baseAddress = reinterpret_cast<std::uintptr_t>(page->start);
            currentAddress = baseAddress + page->offset;
            alignedAddress = Internal::AlignForward(currentAddress, memAlignment);

            padding = static_cast<std::size_t>(alignedAddress - currentAddress);
            requiredSize = padding + size;
        }

        assert(page->offset + requiredSize <= page->capacity);

        page->offset += requiredSize;
        page->usedSize = page->offset;
        page->peakSize = std::max(page->peakSize, page->usedSize);

        return reinterpret_cast<void*>(alignedAddress);
    }


    void ArenaAllocator::Reset()
    {
        for (Page& page : m_pages)
        {
            page.offset = 0;
            page.usedSize = 0;
        }
        m_currentPageIndex = 0;
    }


    ArenaAllocator::Marker ArenaAllocator::SaveMarker() const
    {
        assert(!m_pages.empty());
        assert(m_currentPageIndex < m_pages.size());

        return Marker
        {
            .pageIndex = m_currentPageIndex,
            .offset = m_pages[m_currentPageIndex].offset
        };
    }


    void ArenaAllocator::RollbackToMarker(const Marker marker)
    {
        assert(marker.pageIndex < m_pages.size());
        assert(marker.offset <= m_pages[marker.pageIndex].offset);

        for (std::size_t i = marker.pageIndex + 1; i < m_pages.size(); ++i)
        {
            m_pages[i].offset = 0;
            m_pages[i].usedSize = 0;
        }

        Page& page = m_pages[marker.pageIndex];
        page.offset = marker.offset;
        page.usedSize = marker.offset;

        m_currentPageIndex = marker.pageIndex;
    }


    void ArenaAllocator::PrintUsage() const
    {
        std::printf("Arena Allocator: %zu used / %zu total | peak = %zu | pages = %zu | pageSize = %zu\n",
            GetUsedSize(), GetTotalSize(), GetPeakSize(), GetPageCount(), m_defaultPageSize);
    }


    std::size_t ArenaAllocator::GetUsedSize() const
    {
        std::size_t total = 0;

        for (const Page& page : m_pages)
            total += page.usedSize;

        return total;
    }


    std::size_t ArenaAllocator::GetPeakSize() const
    {
        std::size_t total = 0;

        for (const Page& page : m_pages)
            total += page.peakSize;

        return total;
    }


    std::size_t ArenaAllocator::GetTotalSize() const
    {
        std::size_t total = 0;

        for (const Page& page : m_pages)
            total += page.capacity;

        return total;
    }


    ArenaAllocator::Page& ArenaAllocator::CreatePage(const std::size_t minimumCapacity)
    {
        const std::size_t pageCapacity = Internal::AlignUp(std::max(m_defaultPageSize, minimumCapacity), m_alignment);

        Page page {};
        page.capacity = pageCapacity;

    #ifdef _WIN32
        page.start = static_cast<std::byte*>(_aligned_malloc(page.capacity, m_alignment));
    #else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, m_alignment, page.capacity) != 0)
            ptr = nullptr;

        page.start = static_cast<std::byte*>(ptr);
    #endif

        assert(page.start != nullptr && "ArenaAllocator page allocation failed");

        page.offset = 0;
        page.usedSize = 0;
        page.peakSize = 0;

        m_pages.push_back(page);
        m_currentPageIndex = m_pages.size() - 1;

        return m_pages.back();
    }


    ArenaAllocator::Page& ArenaAllocator::GetCurrentPage()
    {
        assert(!m_pages.empty() && "ArenaAllocator has no page");
        assert(m_currentPageIndex < m_pages.size() && "ArenaAllocator current page index out of bounds");

        return m_pages[m_currentPageIndex];
    }
}