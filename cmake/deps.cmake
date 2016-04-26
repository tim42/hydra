
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# hydra is a vulkan renderer, you'll need vulkan somewhere !
find_package(Vulkan REQUIRED)

# optional support for Reflective
find_package(Reflective)


# deps libs (for exec)
set(PROJ_DEPS_LIBS ${VULKAN_LIBRARY})

# include dirs
set(PROJ_INCLUDE_DIRS ${VULKAN_INCLUDE_DIR})

