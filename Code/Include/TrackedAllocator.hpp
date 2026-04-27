#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>      // GCC std::uint32_t & std::uintptr_t
#include <utility>
#include <vector>

#include "AllocationHeader.hpp"
#include "MemoryAllocator.hpp"
#include "MemoryTracker.hpp"
#include "Utils/MemoryUtils.hpp"


namespace Kayou::Memory
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
        KAYOU_ALWAYS_INLINE void* Alloc(std::size_t size, MemoryTag tag = MemoryTag::General, const std::size_t memAlignment = alignof(std::max_align_t))
        {
            if (size == 0)
                return nullptr;

            assert(std::has_single_bit(memAlignment) && "TrackedAllocator memAlignment must be power of 2");

            const std::size_t totalSize = size + sizeof(Internal::AllocationHeader) + memAlignment - 1;
            const std::size_t requiredAlignment = std::max(memAlignment, alignof(Internal::AllocationHeader));
            void* rawPtr = m_derived.Alloc(totalSize, requiredAlignment);
            if (rawPtr == nullptr)
                return nullptr;

            const std::uintptr_t rawAddress = reinterpret_cast<std::uintptr_t>(rawPtr);
            const std::uintptr_t afterHeader = rawAddress + sizeof(Internal::AllocationHeader);
            const std::uintptr_t userAddress = Internal::AlignForward(afterHeader, memAlignment);

            Internal::AllocationHeader* header = reinterpret_cast<Internal::AllocationHeader*>(userAddress - sizeof(Internal::AllocationHeader));
            header->size = size;
            header->tag = tag;
            header->adjustment = static_cast<std::uint32_t>(userAddress - rawAddress);

        #ifdef KAYOU_DEBUG
            header->magic = Internal::kAllocationHeaderMagic;
        #endif

            void* userPtr = reinterpret_cast<void*>(userAddress);
            MemoryTracker::AddAllocation(userPtr, size, tag);

            if constexpr (ResettableAllocator<Derived>)
                m_activeAllocations.push_back({ userPtr, size, tag });

            return userPtr;
        }


        KAYOU_ALWAYS_INLINE void Free(void* ptr) requires FreeableAllocator<Derived>
        {
            if (ptr == nullptr)
                return;

            const Internal::AllocationHeader* header = Internal::GetAllocationHeader(ptr);

            #ifdef KAYOU_DEBUG
            assert(header->magic == Internal::kAllocationHeaderMagic && "Allocation header corrupted or invalid pointer");
            #endif

            const std::uintptr_t userAddress = reinterpret_cast<std::uintptr_t>(ptr);
            const std::size_t size = header->size;
            const MemoryTag tag = header->tag;
            const std::uint32_t adjustment = header->adjustment;

            assert(adjustment >= sizeof(Internal::AllocationHeader) && "Invalid allocation header adjustment");

            void* rawPtr = reinterpret_cast<void*>(userAddress - adjustment);
            assert(rawPtr != nullptr && "Reconstructed raw pointer is invalid");

            RemoveTrackedAllocation(ptr, size, tag);

            if constexpr (ResettableAllocator<Derived>)
            {
                const auto it = std::find_if(m_activeAllocations.begin(), m_activeAllocations.end(),
                    [ptr](const ActiveAllocation& alloc)
                    {
                        return alloc.ptr == ptr;
                    });

                if (it != m_activeAllocations.end())
                    m_activeAllocations.erase(it);
            }

            m_derived.Free(rawPtr);
        }


        /// Function used to reset the linear allocator
        ///     Because, by concept, a Linear Allocator will free the entire allocated block,
        ///     This will replace any Free() function
        KAYOU_ALWAYS_INLINE void Reset() requires ResettableAllocator<Derived>
        {
            for (const ActiveAllocation& alloc : m_activeAllocations)
                RemoveTrackedAllocation(alloc.ptr, alloc.size, alloc.tag);

            m_activeAllocations.clear();
            m_derived.Reset();
        }


        /// Function used to print the allocator's usage
        KAYOU_ALWAYS_INLINE void PrintUsage() const requires PrintableAllocator<Derived>
        {
            m_derived.PrintUsage();
        }


        /// Non-const getter to access an allocator
        /// @return The desired allocator (not const)
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE Derived& GetAllocator()
        {
            return m_derived;
        }


        /// Const getter to access an allocator
        /// @return The desired allocator (const)
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE const Derived& GetAllocator() const
        {
            return m_derived;
        }



    private:
        struct ActiveAllocation
        {
            void* ptr;
            std::size_t size;
            MemoryTag tag;
        };

        KAYOU_ALWAYS_INLINE void RemoveTrackedAllocation(void* ptr, const std::size_t size, const MemoryTag tag)
        {
            #ifdef KAYOU_DEBUG
            Internal::AllocationHeader* header = Internal::GetAllocationHeader(ptr);
            header->magic = 0u;
            #endif

            MemoryTracker::RemoveAllocation(ptr, size, tag);
        }


        std::vector<ActiveAllocation> m_activeAllocations { };
        Derived m_derived;
    };
}
