
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# hydra is a vulkan renderer, you'll need vulkan somewhere !
find_package(Vulkan REQUIRED)
# we need glslang to build shaders:
find_package(GLSLang REQUIRED)

# We need liburing for async IO
find_package(LibUring REQUIRED)

# We need libmagic for filetypes
find_package(LibMagic REQUIRED)

# LZMA is optional
find_package(LibLZMA)

