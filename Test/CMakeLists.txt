#---------------------------------------#
#            BUILD MODE                 #
#---------------------------------------#

if (NOT BUILD_EXECUTABLE)
    unset(KALLOC_TEST CACHE)
    return()
endif()

# Which test should be enable (only one can be enabled at once)
# Options prefixed with `T` are TrackedAllocators
set(KALLOC_TEST "BENCHMARK" CACHE STRING "Allocator test to run")
set_property(CACHE KALLOC_TEST PROPERTY STRINGS TLINEAR TPOOL STACK TSTACK FREELIST TFREELIST BENCHMARK)

if (KALLOC_TEST STREQUAL "TLINEAR")
    add_executable(KAlloc_TEST "TLinearAllocator.cpp")
elseif (KALLOC_TEST STREQUAL "TPOOL")
    add_executable(KAlloc_TEST "TPoolAllocator.cpp")
elseif (KALLOC_TEST STREQUAL "TSTACK")
    add_executable(KAlloc_TEST "TStackAllocator.cpp")
elseif (KALLOC_TEST STREQUAL "STACK")
    add_executable(KAlloc_TEST "StackAllocator.cpp")
elseif (KALLOC_TEST STREQUAL "FREELIST")
    add_executable(KAlloc_TEST "FreeListAllocator.cpp")
elseif (KALLOC_TEST STREQUAL "TFREELIST")
    add_executable(KAlloc_TEST "TFreeListAllocator.cpp")
elseif (KALLOC_TEST STREQUAL "BENCHMARK")
    add_executable(KAlloc_TEST "Benchmark.cpp")
else ()
    message(FATAL_ERROR "Invalid KALLOC_TEST value")
endif ()

#---------------------------------------#
#                COMPILE MODE           #
#---------------------------------------#
target_compile_features(KAlloc_TEST PUBLIC cxx_std_23)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    target_compile_options(KAlloc_TEST PUBLIC /W4 /WX)
else()
    target_compile_options(KAlloc_TEST PUBLIC -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(KAlloc_TEST PRIVATE KAlloc)

if(MINGW)
    target_link_options(KAlloc_TEST PRIVATE -static-libgcc -static-libstdc++)
endif()