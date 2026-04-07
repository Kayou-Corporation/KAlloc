#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

#include "AllocationHeader.hpp"
#include "MemoryAllocator.hpp"
#include "MemoryTracker.hpp"



namespace Kayou
{
    template <typename Derived>
    class TrackedAllocator
    {
    public:
        template <typename... Args>
        explicit TrackedAllocator(Args&&... args) : m_derived(std::forward<Args>(args)...) { }

        TrackedAllocator(TrackedAllocator&&) = delete;
        TrackedAllocator(const TrackedAllocator&) = delete;
        TrackedAllocator& operator=(TrackedAllocator&&) = delete;
        TrackedAllocator& operator=(const TrackedAllocator&) = delete;


        /// Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param tag [OPTIONAL] A tag used to differentiate allocation blocs - Defaults to MemoryTag::General
        /// @param memAlignment The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new Allocator
        void* Alloc(std::size_t size, MemoryTag tag = MemoryTag::General, const std::size_t memAlignment = alignof(std::max_align_t))
        {
            if (size == 0)
                return nullptr;

            assert(std::has_single_bit(memAlignment) && "memAlignment must be a power of 2");

            const std::size_t totalSize = size + sizeof(AllocationHeader) + memAlignment - 1;
            void* rawPtr = m_derived.Alloc(totalSize, alignof(std::max_align_t));
            if (rawPtr == nullptr)
                return nullptr;

            const std::uintptr_t rawAddress = reinterpret_cast<std::uintptr_t>(rawPtr);
            const std::uintptr_t afterHeader = rawAddress + sizeof(AllocationHeader);
            const std::uintptr_t userAddress = AlignForward(afterHeader, memAlignment);

            AllocationHeader* header = reinterpret_cast<AllocationHeader*>(userAddress - sizeof(AllocationHeader));
            header->size = size;
            header->tag = tag;
            header->adjustment = static_cast<std::uint32_t>(userAddress - rawAddress);

        #ifdef KAYOU_DEBUG
            header->magic = kAllocationHeaderMagic;
        #endif

            void* userPtr = reinterpret_cast<void*>(userAddress);
            MemoryTracker::AddAllocation(userPtr, size, tag);

            if constexpr (ResettableAllocator<Derived>)
                m_allocations.emplace_back(userPtr, size, tag);

            return userPtr;
        }


        void Free(void* ptr) requires FreeableAllocator<Derived>
        {
            if (ptr == nullptr)
                return;

            const AllocationHeader* header = GetAllocationHeader(ptr);
        #ifdef KAYOU_DEBUG
            assert(header->magic == kAllocationHeaderMagic && "Allocation header corrupted or invalid pointer");
        #endif

            const std::uintptr_t userAddress = reinterpret_cast<std::uintptr_t>(ptr);

            const std::size_t size = header->size;
            const MemoryTag tag = header->tag;
            const std::uint32_t adjustment = header->adjustment;
            assert(adjustment >= sizeof(AllocationHeader) && "Invalid allocation header adjustment");

            void* rawPtr = reinterpret_cast<void*>(userAddress - adjustment);
            MemoryTracker::RemoveAllocation(ptr, size, tag);

            if constexpr (ResettableAllocator<Derived>)
            {
                const auto it = std::find_if(m_allocations.begin(), m_allocations.end(),
                    [ptr](const auto& alloc)
                    {
                        return std::get<0>(alloc) == ptr;
                    });

                if (it != m_allocations.end())
                    m_allocations.erase(it);
            }

        #ifdef KAYOU_DEBUG
            AllocationHeader* mutableHeader = GetAllocationHeader(ptr);
            mutableHeader->magic = 0u;
        #endif

            m_derived.Free(rawPtr);
        }


        /// Function used to reset the linear allocator
        ///     Because, by concept, a Linear Allocator will free the entire allocated block,
        ///     This will replace any Free() function
        void Reset() requires ResettableAllocator<Derived>
        {
            for (const auto& [ptr, size, tag] : m_allocations)
                MemoryTracker::RemoveAllocation(ptr, size, tag);

            m_allocations.clear();
            m_derived.Reset();
        }


        /// Function used to print the allocator's usage
        void PrintUsage() const requires PrintableAllocator<Derived>
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
        static std::uintptr_t AlignForward(const std::uintptr_t address, const std::size_t memAlignment)
        {
            return (address + (memAlignment - 1)) & ~(memAlignment - 1);
        }

        std::vector<std::tuple<void*, std::size_t, MemoryTag>> m_allocations { };
        Derived m_derived;
    };
}
