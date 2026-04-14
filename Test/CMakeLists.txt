#---------------------------------------#
#            BUILD MODE                 #
#---------------------------------------#
# Which test should be enable (only one can be enabled at once)
# Options prefixed with `T` are TrackedAllocators
set(KALLOC_TEST "TLINEAR" CACHE STRING "Allocator test to run")
set_property(CACHE KALLOC_TEST PROPERTY STRINGS TLINEAR TPOOL STACK TSTACK)

if (KALLOC_TEST STREQUAL "TLINEAR")
    add_executable(KAlloc_TEST "TLinearAllocator.cpp")
elseif (KALLOC_TEST STREQUAL "TPOOL")
    add_executable(KAlloc_TEST "TPoolAllocator.cpp")
elseif (KALLOC_TEST STREQUAL "TSTACK")
    add_executable(KAlloc_TEST "TStackAllocator.cpp")
elseif (KALLOC_TEST STREQUAL "STACK")
    add_executable(KAlloc_TEST "StackAllocator.cpp")
else()
    message(FATAL_ERROR "Invalid KQLLOC_TEST value")
endif()



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


# Build position independent code.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Disable C and C++ compiler extensions.
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

target_link_libraries(KAlloc_TEST PRIVATE KAlloc)

if(MINGW)
    target_link_options(KAlloc_TEST PRIVATE -static-libgcc -static-libstdc++)
endif()