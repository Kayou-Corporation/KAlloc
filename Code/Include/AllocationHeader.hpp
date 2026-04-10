/// Header used internally to debug allocations and store header information
/// This should not be used by anything outside KAlloc

#pragma once

#include <cstddef>
#include <cstdint>      // GCC std::uint32_t

#include "Utils/Utils.hpp"


namespace Kayou::Memory
{
    enum class MemoryTag : uint8_t;
}


namespace Kayou::Memory::Internal
{
    struct AllocationHeader
    {
        std::size_t size;
        MemoryTag tag;
        std::uint32_t adjustment;

    #ifdef KAYOU_DEBUG
        std::uint32_t magic;
    #endif
    };


#ifdef KAYOU_DEBUG
    constexpr std::uint32_t kAllocationHeaderMagic = 0xDEADC0DEu;
#endif


    KAYOU_ALWAYS_INLINE AllocationHeader* GetAllocationHeader(void* ptr)
    {
        return reinterpret_cast<AllocationHeader*>(reinterpret_cast<std::uintptr_t>(ptr) - sizeof(AllocationHeader));
    }


    KAYOU_ALWAYS_INLINE const AllocationHeader* GetAllocationHeader(const void* ptr)
    {
        return reinterpret_cast<const AllocationHeader*>(reinterpret_cast<std::uintptr_t>(ptr) - sizeof(AllocationHeader));
    }
}
