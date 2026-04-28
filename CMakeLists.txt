#---------------------------------------#
#				  SETUP					#
#---------------------------------------#
cmake_minimum_required(VERSION 3.29.0 FATAL_ERROR)

project(KAlloc VERSION 1.0.0 LANGUAGES CXX)



#---------------------------------------#
#				OPTIONS					#
#---------------------------------------#
option(KALLOC_USE_TRACY "Enable Tracy Integration" OFF)
option(KALLOC_DEBUG_ENABLED "Toggle Internal Debugging Features" OFF)
option(KALLOC_BUILD_EXECUTABLE "Build an executable to live debug the allocator" OFF)



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
file(GLOB_RECURSE KALLOC_SOURCES "Code/Source/*.cpp")



#---------------------------------------#
#				BUILD MODE  			#
#---------------------------------------#

add_library(KAlloc STATIC ${KALLOC_SOURCES})



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



#---------------------------------------#
#           COMPILING OPTIONS           #
#---------------------------------------#
# Toggle tracy internal integration

if (KALLOC_USE_TRACY)
    # Tracy folder include is done inside `Libs/CMakeLists.txt`
    target_link_libraries(KAlloc PUBLIC Tracy::TracyClient)
    target_compile_definitions(KAlloc PUBLIC KAYOU_USE_TRACY)
endif ()


# Toggle debug features
if (KALLOC_DEBUG_ENABLED)
    target_compile_definitions(KAlloc PUBLIC KAYOU_DEBUG)
endif ()



#---------------------------------------#
#        EXPORTS / IMPORTS FOR LIB      #
#---------------------------------------#
# Disable lib prefix
set_target_properties(KAlloc PROPERTIES PREFIX "")

# Set dll name
set_target_properties(KAlloc PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")



#---------------------------------------#
#              ENABLE TESTING           #
#---------------------------------------#
enable_testing()


if (KALLOC_BUILD_EXECUTABLE)
    add_subdirectory(Test)
endif()