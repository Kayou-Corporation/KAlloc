#include "DynamicPoolAllocator.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>      // GCC std::uint32_t & std::uintptr_t
#include <cstdio>
#include <utility>

#include "Utils/MemoryUtils.hpp"

#ifdef _WIN32
    #include <malloc.h>     // MSVC _aligned_malloc & _aligned_free
#else
    #include <cstdlib>      // GCC/CLang std::free & posix_memalign
#endif



namespace Kayou::Memory
{
    DynamicPoolAllocator::DynamicPoolAllocator(const std::size_t blockCapacity, const std::size_t blocksPerPage, const std::size_t memAlignment)
    {
        assert(blockCapacity > 0 && "DynamicPoolAllocator blockCapacity must be > 0");
        assert(blocksPerPage > 0 && "DynamicPoolAllocator blocksPerPage must be > 0");
        assert(std::has_single_bit(memAlignment) && "DynamicPoolAllocator memAlignment must be power of 2");

        m_alignment = memAlignment;
        m_blockCapacity = std::max(blockCapacity, sizeof(FreeNode));
        m_blockStride = Internal::AlignUp(m_blockCapacity, m_alignment);
        m_blocksPerPage = blocksPerPage;

        CreatePage();
    }


    DynamicPoolAllocator::~DynamicPoolAllocator()
    {
        for (Page& page : m_pages)
        {
        #ifdef _WIN32
            _aligned_free(page.start);
        #else
            std::free(page.start);
        #endif
            page.start = nullptr;
            page.freeList = nullptr;
            page.blockStates.clear();
            page.usedBlocks = 0;
            page.peakBlocks = 0;
        }

        m_pages.clear();
        m_totalSize = 0;
    }


    void* DynamicPoolAllocator::Alloc(const std::size_t size, const std::size_t memAlignment)
    {
        if (size == 0)
            return nullptr;

        assert(std::has_single_bit(memAlignment) && "DynamicPoolAllocator memAlignment must be power of 2");

        if (size > m_blockCapacity || memAlignment > m_alignment)
            return nullptr;

        Page* selectedPage = nullptr;
        for (Page& page : m_pages)
        {
            if (page.freeList != nullptr)
            {
                selectedPage = &page;
                break;
            }
        }

        if (selectedPage == nullptr)
            selectedPage = &CreatePage();

        FreeNode* node = selectedPage->freeList;
        assert(node != nullptr && "DynamicPoolAllocator allocation failed");

        selectedPage->freeList = node->next;

        const std::size_t blockIndex = GetBlockIndex(*selectedPage, node);

        #if KAYOU_DEBUG
            assert(selectedPage->blockStates[blockIndex] == BlockState::Free && "DynamicPoolAllocator block already used");
        #endif

        selectedPage->blockStates[blockIndex] = BlockState::Used;
        ++selectedPage->usedBlocks;
        selectedPage->peakBlocks = std::max(selectedPage->peakBlocks, selectedPage->usedBlocks);

        return node;
    }


    void DynamicPoolAllocator::Free(void* ptr)
    {
        if (ptr == nullptr)
            return;

        Page* page = FindOwningPage(ptr);
        assert(page != nullptr && "DynamicPoolAllocator pointer does not belong to any page");

        const std::size_t blockIndex = GetBlockIndex(*page, ptr);

        #if KAYOU_DEBUG
            assert(page->blockStates[blockIndex] == BlockState::Used && "DynamicPoolAllocator block already free or invalid pointer");
        #endif

        page->blockStates[blockIndex] = BlockState::Free;

        FreeNode* node = static_cast<FreeNode*>(ptr);
        node->next = page->freeList;
        page->freeList = node;

        assert(page->usedBlocks > 0 && "DynamicPoolAllocator used block underflow");

        --page->usedBlocks;
    }


    void DynamicPoolAllocator::Reset()
    {
        for (Page& page : m_pages)
            InitPageFreeList(page);
    }


    void DynamicPoolAllocator::PrintUsage() const
    {
        std::printf("DynamicPool Allocator: %zu / %zu blocks used (peak: %zu) | pages = %zu | blockStride = %zu | total = %zu bytes\n",
            GetUsedBlocks(), GetTotalBlockCount(), GetPeakBlocks(), GetPageCount(), m_blockStride, m_totalSize);
    }


    size_t DynamicPoolAllocator::GetUsedBlocks() const
    {
        std::size_t total = 0;
        for (const Page& page : m_pages)
            total += page.usedBlocks;

        return total;
    }


    size_t DynamicPoolAllocator::GetFreeBlocks() const
    {
        return GetTotalBlockCount() - GetUsedBlocks();
    }


    size_t DynamicPoolAllocator::GetPeakBlocks() const
    {
        std::size_t total = 0;
        for (const Page& page : m_pages)
            total += page.peakBlocks;

        return total;
    }


    std::size_t DynamicPoolAllocator::GetTotalBlockCount() const
    {
        return m_pages.size() * m_blocksPerPage;
    }


    DynamicPoolAllocator::Page& DynamicPoolAllocator::CreatePage()
    {
        Page page {};
        page.blockStates.resize(m_blocksPerPage, BlockState::Free);

        const std::size_t pageSize = m_blockStride * m_blocksPerPage;

        #ifdef _WIN32
            page.start = static_cast<std::byte*>(_aligned_malloc(pageSize, m_alignment));
        #else
            void* ptr = nullptr;
            if (posix_memalign(&ptr, m_alignment, pageSize) != 0)
                ptr = nullptr;

            page.start = static_cast<std::byte*>(ptr);
        #endif

        assert(page.start != nullptr && "DynamicPoolAllocator allocation failed");

        page.freeList = nullptr;
        page.usedBlocks = 0;
        page.peakBlocks = 0;

        InitPageFreeList(page);

        m_totalSize += pageSize;

        m_pages.push_back(std::move(page));

        return m_pages.back();
    }


    void DynamicPoolAllocator::InitPageFreeList(Page& page)
    {
        page.freeList = nullptr;

        for (std::size_t i = 0; i < m_blocksPerPage; ++i)
        {
            FreeNode* node = reinterpret_cast<FreeNode*>(page.start + i * m_blockStride);
            node->next = page.freeList;
            page.freeList = node;
            page.blockStates[i] = BlockState::Free;
        }
        page.usedBlocks = 0;
    }


    DynamicPoolAllocator::Page* DynamicPoolAllocator::FindOwningPage(const void* ptr)
    {
        const std::byte* address = static_cast<const std::byte*>(ptr);

        for (Page& page : m_pages)
        {
            const std::byte* pageStart = page.start;
            const std::byte* pageEnd = pageStart + (m_blockStride * m_blocksPerPage);

            if (address >= pageStart && address < pageEnd)
                return &page;
        }
        return nullptr;
    }


    const DynamicPoolAllocator::Page* DynamicPoolAllocator::FindOwningPage(const void* ptr) const
    {
        const std::byte* address = static_cast<const std::byte*>(ptr);

        for (const Page& page : m_pages)
        {
            const std::byte* pageStart = page.start;
            const std::byte* pageEnd = pageStart + (m_blockStride * m_blocksPerPage);

            if (address >= pageStart && address < pageEnd)
                return &page;
        }
        return nullptr;
    }


    std::size_t DynamicPoolAllocator::GetBlockIndex(const Page& page, const void* ptr) const
    {
        const std::byte* address = static_cast<const std::byte*>(ptr);

        assert(address >= page.start && "DynamicPoolAllocator pointer out of page bounds");
        assert(address < page.start + (m_blockStride * m_blocksPerPage) && "DynamicPoolAllocator pointer out of page bounds");

        const std::uintptr_t offset = static_cast<std::uintptr_t>(address - page.start);

        assert((offset % m_blockStride) == 0 && "DynamicPoolAllocator pointer not aligned to block stride");

        return static_cast<std::size_t>(offset / m_blockStride);
    }
}
