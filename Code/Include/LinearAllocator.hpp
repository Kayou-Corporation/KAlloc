#pragma once

#include "MemoryAllocator.hpp"
#include "Utils/Utils.hpp"


namespace Kayou::Memory
{
    class LinearAllocator
    {
    public:
        /// @brief Constructor used to register a new Linear Allocator
        /// @param size The allocation size
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        explicit LinearAllocator(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        ~LinearAllocator();

        LinearAllocator(const LinearAllocator&) = delete;
        LinearAllocator(LinearAllocator&&) = delete;
        LinearAllocator& operator=(const LinearAllocator&) = delete;
        LinearAllocator& operator=(LinearAllocator&&) = delete;

        /// @brief Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new allocation
        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));

        /// @brief Function used to reset the linear allocator
        ///     because, by concept, a Linear Allocator will free the entire allocated block.
        ///     This will replace any Free() function
        void Reset();

        /// @brief Function used to print the allocator's usage
        void PrintUsage() const;

        /// @return The size of the memory currently in use, in bytes
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetUsedSize() const            { return m_usedSize; }

        /// @return the current peak allocation value
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetPeakSize() const            { return m_peakSize; }

        /// @return The total size of the allocator (free and used), in bytes
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetTotalSize() const           { return m_totalSize; }

        /// @return The current offset (alignment) of the allocator
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetOffset() const              { return m_offset; }


    private:
        std::byte* m_start = nullptr;
        std::size_t m_offset = 0;

        std::size_t m_totalSize = 0;
        std::size_t m_usedSize = 0;
        std::size_t m_peakSize = 0;
    };
}
