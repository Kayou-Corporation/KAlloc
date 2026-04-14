#pragma once

#include "Utils/Utils.hpp"

#ifdef KAYOU_USE_TRACY
    #include "Tracy/Tracy.hpp"

    #define KZoneScoped ZoneScoped
    #define KFrameMark FrameMark
#endif


namespace Kayou::Memory::Profiler
{
#ifdef KAYOU_USE_TRACY

    /// @brief Wrapper function for TracyAllocN()
    /// @param ptr The pointer you want to profile using Tracy
    /// @param size The size of the allocated block used by Tracy to generate statistics
    /// @param tag A tag used to differentiate the different allocations
    KAYOU_ALWAYS_INLINE void Alloc(const void* ptr, const std::size_t size, const char* tag = "Unspecified")
    {
        TracyAllocN(ptr, size, tag);
    }


    /// @brief Wrapper function for TracyFreeN()
    /// @param ptr The pointer you want Tracy to stop profiling
    /// @param tag The tag assigned to this pointer when allocating it
    KAYOU_ALWAYS_INLINE void Free(const void* ptr, const char* tag = "Unspecified")
    {
        TracyFreeN(ptr, tag);
    }


#else

    #define KZoneScoped ((void)0)
    #define KFrameMark ((void)0)

    /// Helper function used when `USE_TRACY` is enabled in the CMakeLists.txt to track allocations inside Tracy
    void Alloc([[maybe_unused]] const void* ptr, [[maybe_unused]] const size_t size, [[maybe_unused]] const char* tag = "Unspecified") { }

    /// Helper function used when `USE_TRACY` is enabled in the CMakeLists.txt to untrack allocations inside Tracy
    void Free([[maybe_unused]] const void* ptr, [[maybe_unused]] const char* tag = "Unspecified") { }

#endif
}
