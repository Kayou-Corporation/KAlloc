#pragma once

#ifdef KAYOU_USE_TRACY
    #include "Tracy/Tracy.hpp"
    #include "Tracy/TracyC.h"

    #define KZoneScoped ZoneScoped;
    #define KFrameMark FrameMark;
#endif



namespace Kayou::Profiler
{
#ifdef KAYOU_USE_TRACY

    /// Wrapper function for TracyAllocN()
    /// @param ptr The pointer you want to profile using Tracy
    /// @param size The size of the allocated block used by Tracy to generate statistics
    /// @param tag A tag used to differentiate the different allocations
    inline void Alloc(const void* ptr, const std::size_t size, const char* tag = "Unspecified")
    {
        TracyAllocN(ptr, size, tag);
    }


    /// Wrapper function for TracyFreeN()
    /// @param ptr The pointer you want Tracy to stop profiling
    /// @param tag The tag assigned to this pointer when allocating it
    inline void Free(const void* ptr, const char* tag = "Unspecified")
    {
        TracyFreeN(ptr, tag);
    }


#else

    #define KZoneScoped ((void)0)
    #define KFrameMark ((void)0)

    /// Helper function used when `USE_TRACY` is enabled in the CMakeLists.txt to track allocations inside Tracy
    inline void Alloc([[maybe_unused]] const void* ptr, [[maybe_unused]] const std::size_t size, [[maybe_unused]] const char* tag = "Unspecified") { }

    /// Helper function used when `USE_TRACY` is enabled in the CMakeLists.txt to untrack allocations inside Tracy
    inline void Free([[maybe_unused]] const void* ptr, [[maybe_unused]] const char* tag = "Unspecified") { }

    /// Helper function used when `USE_TRACY` is enabled in the CMakeLists.txt to register a new tracking zone
    inline void KZoneScoped() { }

    /// Helper function used when `USE_TRACY` is enabled in the CMakeLists.txt to register a new FrameMark
    inline void KFrameMark() { }

#endif
}