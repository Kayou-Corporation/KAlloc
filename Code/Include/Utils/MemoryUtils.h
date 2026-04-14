#pragma once
#include <cstddef>

#include "Utils.hpp"


namespace Kayou::Memory::Internal
{
    /// @brief Internal helper function used to align the size
    /// @param value The size to align
    /// @param alignment The desired memory alignment (always a multiple of 2)
    /// @return The new address for the pointer
    KAYOU_ALWAYS_INLINE constexpr std::size_t AlignUp(const std::size_t value, const std::size_t alignment)
    {
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    /// @brief Internal helper function used to align the specified pointer address
    /// @param address The pointer address to align
    /// @param alignment The desired memory alignment (always a multiple of 2)
    /// @return The new address for the pointer
    KAYOU_ALWAYS_INLINE constexpr std::size_t AlignForward(const std::size_t address, const std::size_t alignment)
    {
        return (address + (alignment - 1)) & ~(alignment - 1);
    }
}
