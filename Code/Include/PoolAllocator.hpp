#pragma once
#include <cstddef>


namespace Kayou
{
    class PoolAllocator
    {
    public:
        PoolAllocator(std::size_t objectSize, std::size_t objectCount, std::size_t memAlignment = alignof(std::max_align_t));
        ~PoolAllocator();

        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        void Free(void* ptr);
        void Reset();
        void PrintUsage() const;

        [[nodiscard]] inline size_t GetObjectSize() const { return m_objectSize; }
        [[nodiscard]] inline size_t GetObjectCount() const { return m_objectCount; }
        [[nodiscard]] inline size_t GetAlignment() const { return m_alignment; }
        [[nodiscard]] inline size_t GetUsedBlocks() const { return m_usedBlocks; }
        [[nodiscard]] inline size_t GetFreeBlocks() const { return m_objectCount - m_usedBlocks; }
        [[nodiscard]] inline size_t GetPeakBlocks() const { return m_peakBlocks; }
        [[nodiscard]] inline size_t GetTotalSize() const { return m_totalSize; }

        PoolAllocator(const PoolAllocator&) = delete;
        PoolAllocator(PoolAllocator&&) = delete;
        PoolAllocator& operator=(const PoolAllocator&) = delete;
        PoolAllocator& operator=(PoolAllocator&&) = delete;


    private:
        struct FreeNode
        {
            FreeNode* m_next = nullptr;
        };

        static size_t AlignUp(size_t size, size_t memAlignment);
        void InitFreeList();

        std::byte* m_start = nullptr;
        FreeNode* m_freeList = nullptr;

        std::size_t m_objectSize = 0;    // Requested size
        std::size_t m_blockSize = 0;     // Real (aligned) block size
        std::size_t m_objectCount = 0;
        std::size_t m_alignment = 0;
        std::size_t m_totalSize = 0;

        std::size_t m_usedBlocks = 0;
        std::size_t m_peakBlocks = 0;
    };
}
