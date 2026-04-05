#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "MemoryTracker.hpp"



namespace Kayou
{
    template <typename Derived>
    class TrackedAllocator
    {
    public:
        template <typename... Args>
        explicit TrackedAllocator(Args&&... args) : m_derived(std::forward<Args>(args)...) { }

        /// Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param memAlignment The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new Allocator
        void* Alloc(std::size_t size, MemoryTag tag = MemoryTag::General, std::size_t memAlignment = alignof(std::max_align_t))
        {
            void* ptr = m_derived.Alloc(size, memAlignment);
            if (ptr && size > 0)
            {
                m_allocations.emplace_back(ptr, size, tag);
                MemoryTracker::AddAllocation(ptr, size, tag);
            }
            return ptr;
        }

        /// Function used to reset the linear allocator
        ///     Because, by concept, a Linear Allocator will free the entire allocated block,
        ///     This will replace any Free() function
        void Reset()
        {
            for (auto& [ptr, size, tag] : m_allocations)
                MemoryTracker::RemoveAllocation(ptr, size, tag);

            m_allocations.clear();
            m_derived.Reset();
        }

        /// Function used to print the allocator's usage
        void PrintUsage() const
        {
            m_derived.PrintUsage();
        }


    private:
        std::vector<std::tuple<void*, std::size_t, MemoryTag>> m_allocations { };
        Derived m_derived = nullptr;
    };
}
