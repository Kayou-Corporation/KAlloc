# --------------------------------------- #
#               CMAKE CONFIG              #
# --------------------------------------- #

# Define minimum CMake version required
cmake_minimum_required(VERSION 3.29.0)



project(MemoryAllocator VERSION 1.0.0 LANGUAGES CXX)



#---------------------------------------#
#            BUILD MODE                 #
#---------------------------------------#

add_executable(KAlloc_TEST "Main.cpp")



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