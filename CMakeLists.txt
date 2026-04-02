#---------------------------------------#
#				SETUP					#
#---------------------------------------#
cmake_minimum_required(VERSION 3.29.0 FATAL_ERROR)

project(KAlloc VERSION 1.0.0 LANGUAGES CXX)



# --------------------------------------- #
#              FIND PACKAGES              #
# --------------------------------------- #
add_subdirectory(Libs)

# Output directories
set(OUTPUT_DIR_BASE "${CMAKE_SOURCE_DIR}/Builds/${CMAKE_CXX_COMPILER_ID}")

message(STATUS "[${PROJECT_NAME}]: Base output directory: ${OUTPUT_DIR_BASE}")

# Use generator expressions directly in output variables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR_BASE}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR_BASE}/$<CONFIG>/libs")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR_BASE}/$<CONFIG>/libs")



#---------------------------------------#
#				SOURCES					#
#---------------------------------------#
file(GLOB_RECURSE KAlloc_SOURCES "Code/Source/*.cpp")



#---------------------------------------#
#				BUILD MODE  			#
#---------------------------------------#
option(KAlloc_BUILD_STATIC "Build static libraries" ON)

if(KAlloc_BUILD_STATIC)
    add_library(KAlloc STATIC ${KAlloc_SOURCES})
else()
    add_library(KAlloc SHARED ${KAlloc_SOURCES})
endif()



#---------------------------------------#
#                INCLUDE                #
#---------------------------------------#
target_include_directories(KAlloc PUBLIC "Code/Include")

#---------------------------------------#
#                COMPILE MODE           #
#---------------------------------------#
target_compile_features(KAlloc PUBLIC cxx_std_23)

if (MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    target_compile_options(KAlloc PUBLIC /W4 /WX)
else ()
    target_compile_options(KAlloc PUBLIC -Wall -Wextra -Wpedantic -Werror)
endif ()

# Build position independent code.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Disable C and C++ compiler extensions.
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_link_libraries(KAlloc PUBLIC Tracy::TracyClient)



#---------------------------------------#
#        EXPORTS / IMPORTS FOR LIB      #
#---------------------------------------#
# Disable lib prefix
set_target_properties(KAlloc PROPERTIES PREFIX "")

# Set dll name
set_target_properties(KAlloc PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")



# --------------------------------------- #
#              ENABLE TESTING             #
# --------------------------------------- #
# Enable CTests
enable_testing()

option(BUILD_EXECUTABLE "Build an executable to live debug the allocator" ON)

if (BUILD_EXECUTABLE)
    add_subdirectory(Test)
endif ()