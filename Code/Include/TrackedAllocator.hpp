#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>      // GCC std::uint32_t & std::uintptr_t
#include <unordered_map>
#include <utility>

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


        /// @brief Function used to register a new allocation
        /// @param size The size of the allocation
        /// @param tag [OPTIONAL] A tag used to differentiate allocation blocks - Defaults to MemoryTag::General
        /// @param memAlignment The desired memory alignment (must always be a multiple-of-two)
        /// @return A pointer to the new Allocator
        KAYOU_ALWAYS_INLINE void* Alloc(std::size_t size, MemoryTag tag = MemoryTag::General, const std::size_t memAlignment = alignof(std::max_align_t))
        {
            if (size == 0)
                return nullptr;

            assert(std::has_single_bit(memAlignment) && "TrackedAllocator memAlignment must be power of 2");

            const std::size_t requiredAlignment = std::max(memAlignment, alignof(Internal::AllocationHeader));
            const std::size_t totalSize = size + sizeof(Internal::AllocationHeader) + requiredAlignment - 1;

            void* rawPtr = m_derived.Alloc(totalSize, requiredAlignment);
            if (rawPtr == nullptr)
                return nullptr;

            const std::uintptr_t rawAddress = reinterpret_cast<std::uintptr_t>(rawPtr);
            const std::uintptr_t afterHeader = rawAddress + sizeof(Internal::AllocationHeader);
            const std::uintptr_t userAddress = Internal::AlignForward(afterHeader, requiredAlignment);

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
                m_activeAllocations.emplace(userPtr, ActiveAllocation{ size, tag });

            return userPtr;
        }


        /// @brief Function used to free any FreeableAllocator's allocation
        /// @param ptr Pointer to the allocation to free
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
                const auto it = m_activeAllocations.find(ptr);

                #ifdef KAYOU_DEBUG
                assert(it != m_activeAllocations.end() && "TrackedAllocator::Free unknown pointer");
                #endif

                if (it != m_activeAllocations.end())
                    m_activeAllocations.erase(it);
            }

            m_derived.Free(rawPtr);
        }


        /// @brief Function used to reset the linear allocator
        ///        Because, by concept, a Linear Allocator will free the entire allocated block,
        ///        This will replace any Free() function
        KAYOU_ALWAYS_INLINE void Reset() requires ResettableAllocator<Derived>
        {
            for (const auto& [ptr, alloc] : m_activeAllocations)
                RemoveTrackedAllocation(ptr, alloc.size, alloc.tag);

            m_activeAllocations.clear();
            m_derived.Reset();
        }


        /// @brief Function used to print the allocator's usage
        KAYOU_ALWAYS_INLINE void PrintUsage() const requires PrintableAllocator<Derived>    { m_derived.PrintUsage(); }

        /// @return The desired allocator (not const)
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE Derived& GetAllocator()                        { return m_derived; }

        /// @return The desired allocator (const)
        KAYOU_NO_DISCARD KAYOU_ALWAYS_INLINE const Derived& GetAllocator() const            { return m_derived; }



    private:
        struct ActiveAllocation
        {
            std::size_t size    = 0;
            MemoryTag tag       = MemoryTag::General;
        };

        KAYOU_ALWAYS_INLINE void RemoveTrackedAllocation(void* ptr, const std::size_t size, const MemoryTag tag)
        {
            #ifdef KAYOU_DEBUG
            Internal::AllocationHeader* header = Internal::GetAllocationHeader(ptr);
            header->magic = 0u;
            #endif

            MemoryTracker::RemoveAllocation(ptr, size, tag);
        }


        std::unordered_map<void*, ActiveAllocation> m_activeAllocations { };
        Derived m_derived;
    };
}
