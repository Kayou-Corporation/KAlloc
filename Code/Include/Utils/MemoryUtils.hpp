#pragma once
#include <cstddef>

#include "Utils.hpp"


namespace Kayou::Memory::Internal
{
    /// @brief Aligns a size value up to the specified alignment
    /// @param value The size to align
    /// @param alignment The desired memory alignment (must be power of 2)
    /// @return The aligned size (greater than or equal to value)
    KAYOU_ALWAYS_INLINE constexpr std::size_t AlignUp(const std::size_t value, const std::size_t alignment)
    {
        return (value + (alignment - 1)) & ~(alignment - 1);
    }

    /// @brief Aligns an address forward to the specified alignment
    /// @param address The address to align (typically obtained from a pointer cast to std::uintptr_t)
    /// @param alignment The desired memory alignment (must be power of 2)
    /// @return The aligned address (greater than or equal to address)
    KAYOU_ALWAYS_INLINE constexpr std::size_t AlignForward(const std::size_t address, const std::size_t alignment)
    {
        return (address + (alignment - 1)) & ~(alignment - 1);
    }
}
