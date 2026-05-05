#---------------------------------------#
#            BUILD MODE                 #
#---------------------------------------#

if (NOT KALLOC_BUILD_EXECUTABLE)
    unset(KALLOC_TEST_MODE CACHE)
    return()
endif()

# Which test should be enable (only one can be enabled at once)
# Options prefixed with `T` are TrackedAllocators
set(KALLOC_TEST_MODE "BENCHMARK" CACHE STRING "Allocator test to run")
set_property(CACHE KALLOC_TEST_MODE PROPERTY STRINGS BENCHMARK STACK FREELIST TLSF DYNAMICPOOL ARENA TARENA TDYNAMICPOOL TLINEAR TSTATICPOOL TSTACK TFREELIST TTLSF)

if (KALLOC_TEST_MODE STREQUAL "BENCHMARK")
    add_executable(KAlloc_Test "Benchmark.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "STACK")
    add_executable(KAlloc_Test "StackAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "FREELIST")
    add_executable(KAlloc_Test "FreeListAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "TLSF")
    add_executable(KAlloc_Test "TLSFAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "DYNAMICPOOL")
    add_executable(KAlloc_Test "DynamicPoolAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "ARENA")
    add_executable(KAlloc_Test "ArenaAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "TARENA")
    add_executable(KAlloc_Test "TArenaAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "TDYNAMICPOOL")
    add_executable(KAlloc_Test "TDynamicPoolAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "TLINEAR")
    add_executable(KAlloc_Test "TLinearAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "TSTATICPOOL")
    add_executable(KAlloc_Test "TStaticPoolAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "TSTACK")
    add_executable(KAlloc_Test "TStackAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "TFREELIST")
    add_executable(KAlloc_Test "TFreeListAllocator.cpp")
elseif (KALLOC_TEST_MODE STREQUAL "TTLSF")
    add_executable(KAlloc_Test "TTLSFAllocator.cpp")
else ()
    message(FATAL_ERROR "Invalid KALLOC_TEST_MODE value")
endif ()

#---------------------------------------#
#                COMPILE MODE           #
#---------------------------------------#
target_compile_features(KAlloc_Test PUBLIC cxx_std_23)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    target_compile_options(KAlloc_Test PUBLIC /W4 /WX)
else()
    target_compile_options(KAlloc_Test PUBLIC -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(KAlloc_Test PRIVATE KAlloc)

if(MINGW)
    target_link_options(KAlloc_Test PRIVATE -static-libgcc -static-libstdc++)
endif()