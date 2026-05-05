#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Utils/Utils.hpp"



namespace Kayou::Memory
{
    class ArenaAllocator
    {
    public:
        /// @brief Constructor used to create a new Arena Allocator
        /// @param pageSize The size of each memory page to be allocated from the system (must be > 0)
        /// @param memAlignment [OPTIONAL] The desired memory alignment for all allocations (must always be a multiple-of-two)
        explicit ArenaAllocator(std::size_t pageSize, std::size_t memAlignment = alignof(std::max_align_t));
        ~ArenaAllocator();

        ArenaAllocator(const ArenaAllocator&) = delete;
        ArenaAllocator(ArenaAllocator&&) = delete;
        ArenaAllocator& operator=(const ArenaAllocator&) = delete;
        ArenaAllocator& operator=(ArenaAllocator&&) = delete;

        /// @brief Allocates temporary memory from the arena
        ///        Individual frees are not supported
        ///        Use Reset() or RollbackToMarker() to free memory
        /// @param size The size of the allocation
        /// @param memAlignment [OPTIONAL] The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new allocation
        void* Alloc(std::size_t size, std::size_t memAlignment = alignof(std::max_align_t));

        /// @brief Resets all pages and makes all arena allocations invalid
        ///        Memory pages are kept for reuse
        void Reset();

        struct Marker
        {
            std::size_t pageIndex = 0;
            std::size_t offset = 0;
        };

        /// @brief Saves the current allocation state of the arena and returns a marker that can be used to roll back to it later
        KAYOU_NO_DISCARD Marker SaveMarker() const;

        /// @brief Rolls back the arena to a previously saved marker, invalidating all allocations made after that marker
        void RollbackToMarker(Marker marker);

        /// @brief Prints arena usage
        void PrintUsage() const;

        /// @return The page size used by the arena
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetDefaultPageSize() const     { return m_defaultPageSize; }

        /// @return The number of pages currently allocated by the arena
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetPageCount() const           { return m_pages.size(); }

        /// @return The memory alignment used by the arena
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE std::size_t GetAlignment() const           { return m_alignment; }

        /// @return The total used size across all pages
        KAYOU_NO_DISCARD std::size_t GetUsedSize() const;

        /// @return The peak used size across all pages
        KAYOU_NO_DISCARD std::size_t GetPeakSize() const;

        /// @return The total capacity across all pages
        KAYOU_NO_DISCARD std::size_t GetTotalSize() const;

    private:
        struct Page
        {
            std::byte* start = nullptr;

            std::size_t offset = 0;
            std::size_t usedSize = 0;
            std::size_t peakSize = 0;
            std::size_t capacity = 0;
        };

        Page& CreatePage(std::size_t minimumCapacity);
        Page& GetCurrentPage();

        std::vector<Page> m_pages {};

        std::size_t m_defaultPageSize = 0;
        std::size_t m_alignment = 0;
        std::size_t m_currentPageIndex = 0;
    };
}