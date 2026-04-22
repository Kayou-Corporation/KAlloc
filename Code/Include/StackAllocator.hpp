#pragma once

#include <cstddef>

#include "Utils/Utils.hpp"


namespace Kayou::Memory
{
    class StackAllocator
    {
    public:

        /// @brief Constructor used to register a new Stack Allocator
        /// @param size The allocation size
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        explicit StackAllocator(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        ~StackAllocator();

        StackAllocator(const StackAllocator&) = delete;
        StackAllocator(StackAllocator&&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;
        StackAllocator& operator=(StackAllocator&&) = delete;

        /// @brief Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new allocation
        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));

        /// @brief Function used to free the selected pointer
        /// @param ptr The pointer that should be freed
        void Free(void* ptr);

        /// @brief Function used to free the selected pointer
        /// @param ptr The pointer that should be freed
        void Pop(void* ptr);

        /// @brief Function used to reset the stack allocator
        void Reset();

        /// @brief Function used to print the allocator's usage
        void PrintUsage() const;

        /// @return The sized used by the allocator
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetUsedSize() const    { return m_usedSize; }

        /// @return The peak used size reached by the allocator
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetPeakSize() const    { return m_peakSize; }

        /// @return The total size of the allocator (free & used)
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetTotalSize() const   { return m_totalSize; }

        /// @return The memory alignment of the allocator
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetOffset() const      { return m_offset; }


    private:
        struct AllocationHeader
        {
            std::size_t previousOffset = 0;
            std::size_t allocationOffset = 0;

            #ifdef KAYOU_DEBUG
            std::uint32_t magic = 0;
            #endif
        };

        #ifdef KAYOU_DEBUG
        inline static constexpr std::uint32_t kStackAllocationHeaderMagic = 0xBAADF00Du;
        #endif

        std::byte* m_start = nullptr;
        std::size_t m_offset = 0;
        std::size_t m_alignment = 0;
        std::size_t m_totalSize = 0;
        std::size_t m_usedSize = 0;
        std::size_t m_peakSize = 0;
    };
}
