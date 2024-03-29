##
## CMAKE file for neam/...
##

cmake_minimum_required(VERSION 3.11)

project(hydra VERSION 0.0.1)

message(STATUS "-----------------------------------------------")
message(STATUS "            ${CMAKE_PROJECT_NAME}")
message(STATUS "-----------------------------------------------")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(USE_TRACY "Enable Tracy support" OFF)

if (${USE_TRACY})
  message(STATUS "Profiling with Tracy enabled")
  #add_compile_definitions(TRACY_ENABLE=1 TRACY_ON_DEMAND=1 TRACY_NO_SAMPLING=1)
  add_compile_definitions(TRACY_ENABLE=1 TRACY_NO_SAMPLING=1)
else()
  message(STATUS "Profiling with Tracy disabled")
endif()

# dependencies:
include(cmake/deps.cmake)

if (${FREETYPE_FOUND})
  add_compile_definitions(IMGUI_ENABLE_FREETYPE=1)
endif()
add_compile_definitions(IMGUI_USER_CONFIG=<imgui_hydra_config.h>)

# build / configure the third_party libs
add_subdirectory(third_party/)

# project flags:
include(cmake/flags.cmake)

if (${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
  message(WARNING "when checking: '${CMAKE_CURRENT_BINARY_DIR}' is the same as '${PROJECT_SOURCE_DIR}'")
  message(WARNING "you shhould create a build folder in order to build ${PROJ_NAME} !!!")
endif()

# global setup:
add_compile_definitions(N_HYDRA_RESOURCES_OBFUSCATE=1) # obfuscate the index and the resources. (require an index rebuild / res repack)
add_compile_definitions(N_RLE_VERBOSE_FAILS=1)
add_compile_definitions(N_RES_MAX_SIZE_TO_EMBED=64) # resources <= this size will be written in the index and always be in memory
add_compile_definitions(N_ALLOW_DEBUG=false)
add_compile_definitions(N_DISABLE_CHECKS=false)
add_compile_definitions(N_ASYNC_USE_TASK_MANAGER=true)
if (LIBLZMA_FOUND)
    message(STATUS "found liblzma2, using resource compression")
    add_compile_definitions(N_RES_LZMA_COMPRESSION=true) # compress resources using liblzma. (require an index rebuild / res repack)
else()
    message(STATUS "liblzma2 not found, deactivating resource compression")
    add_compile_definitions(N_RES_LZMA_COMPRESSION=false) # compress resources using liblzma. (require an index rebuild / res repack)
endif()

# build a static lib
add_subdirectory(${CMAKE_PROJECT_NAME})

# build the tools
add_subdirectory(tools/)

# build the samples
add_subdirectory(samples/)

