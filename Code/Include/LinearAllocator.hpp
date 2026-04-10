#pragma once

#include "MemoryAllocator.hpp"
#include "Utils/Utils.hpp"


namespace Kayou::Memory
{
    class LinearAllocator
    {
    public:

        /// Constructor used to register a new Linear Allocator
        /// @param size The allocation size
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        explicit LinearAllocator(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));
        ~LinearAllocator();

        /// Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new Allocator
        void* Alloc(std::size_t size, std::size_t memAlignment);

        /// Function used to reset the linear allocator
        ///     Because, by concept, a Linear Allocator will free the entire allocated block,
        ///     This will replace any Free() function
        void Reset();

        /// Function used to print the allocator's usage
        void PrintUsage() const;

        /// Getter for the used size
        /// @return The used size of the allocator
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetUsedSize() const { return m_usedSize; }

        /// Getter for the offset (memory alignment)
        /// @return The current offset (alignment) of the allocator
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetOffset() const { return m_offset; }

        /// Getter for the peak allocation
        /// @return the current peak allocation value
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetPeakSize() const { return m_peakSize; }

        /// Getter for the total allocator size
        /// @return The allocator's total size
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetTotalSize() const { return m_totalSize; }


    private:

        /// Internal helper function used to align memory internally
        /// @param ptrAddress The pointer's address
        /// @param memAlignment The desired memory alignment (always a multiple of 2)
        /// @return The new address for the pointer
        KAYOU_ALWAYS_INLINE static std::size_t AlignForward(std::size_t ptrAddress, std::size_t memAlignment)
        {
            return (ptrAddress + (memAlignment - 1)) & ~ (memAlignment - 1);
        }

        std::byte* m_start = nullptr;
        std::size_t m_offset = 0;

        std::size_t m_totalSize = 0;
        std::size_t m_usedSize = 0;
        std::size_t m_peakSize = 0;
    };
}
