#pragma once

#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>
#include <algorithm>

#include "MemoryTracker.hpp"



namespace Kayou
{
    template <typename Derived>
    class TrackedAllocator
    {
    public:
        template <typename... Args>
        explicit TrackedAllocator(Args&&... args) : m_derived(std::forward<Args>(args)...) { }

        TrackedAllocator(const TrackedAllocator&) = delete;
        TrackedAllocator(TrackedAllocator&&) = delete;
        TrackedAllocator& operator=(const TrackedAllocator&) = delete;


        /// Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param tag [OPTIONAL] A tag used to differentiate allocation blocs - Defaults to MemoryTag::General
        /// @param memAlignment The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new Allocator
        void* Alloc(std::size_t size, MemoryTag tag = MemoryTag::General, std::size_t memAlignment = alignof(std::max_align_t))
        {
            void* ptr = m_derived.Alloc(size, memAlignment);
            if (ptr != nullptr && size > 0)
            {
                m_allocations.emplace_back(ptr, size, tag);
                MemoryTracker::AddAllocation(ptr, size, tag);
            }
            return ptr;
        }


        void Free(const void* ptr) requires FreeableAllocator<Derived>
        {
            if (ptr == nullptr)
                return;

            const auto it = std::find_if(m_allocations.begin(), m_allocations.end(), [ptr](const auto& alloc)
                {
                    return std::get<0>(alloc) == ptr;
                });

            if (it != m_allocations.end())
            {
                MemoryTracker::RemoveAllocation(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
                m_allocations.erase(it);
            }
            m_derived.Free(ptr);
        }


        /// Function used to reset the linear allocator
        ///     Because, by concept, a Linear Allocator will free the entire allocated block,
        ///     This will replace any Free() function
        void Reset() requires ResettableAllocator<Derived>
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


        [[nodiscard]] Derived& Allocator()
        {
            return m_derived;
        }


        [[nodiscard]] const Derived& GetAllocator() const
        {
            return m_derived;
        }



    private:
        std::vector<std::tuple<void*, std::size_t, MemoryTag>> m_allocations { };
        Derived m_derived;
    };
}
