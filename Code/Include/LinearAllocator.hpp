#pragma once

#include "MemoryAllocator.hpp"


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

        /// Helper function used to get the size used by allocator
        /// @return The previous used size of the allocator
        [[nodiscard]] std::size_t LastAllocSize() const;

        /// Helper function used to get the offset (alignment) of the allocator
        /// @return The current offset (alignment) of the allocator
        [[nodiscard]] std::size_t GetOffset() const { return m_offset; }


    private:

        /// Internal helper function used to align memory internally
        /// @param ptrAddress The pointer's address
        /// @param memAlignment The desired memory alignment (always a multiple of 2)
        /// @return The new address for the pointer
        static std::size_t AlignForward(std::size_t ptrAddress, std::size_t memAlignment);

        /// Internal function used to track both the current allocation size and the total peak usage
        /// @param size The allocation size
        void TrackAlloc(std::size_t size);

        std::byte* m_start = nullptr;
        std::size_t m_offset = 0;

        std::size_t m_totalSize = 0;
        std::size_t m_usedSize = 0;
        std::size_t m_peakSize = 0;
    };
}