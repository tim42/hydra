

# build GLFW (only what is realy needed)
set(GLFW_BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS FALSE CACHE BOOL "" FORCE)
set(GLFW_INSTALL FALSE CACHE BOOL "" FORCE)
add_subdirectory("./glfw")


# build LodePNG (create a static library)
set(LODEPNG_NAME "lodePNG")
set(LODEPNG_LIBRARY ${LODEPNG_NAME} PARENT_SCOPE)
get_filename_component(LODEPNG_DIR "./lodePNG/" ABSOLUTE)
set(LODEPNG_INCLUDE_DIR ${LODEPNG_DIR} PARENT_SCOPE)
set(LODEPNG_SOURCES "./lodePNG/lodepng.cpp")
add_library(${LODEPNG_NAME} STATIC ${LODEPNG_SOURCES})

# GLM
get_filename_component(GLM_DIR "./glm/" ABSOLUTE)
set(GLM_INCLUDE_DIR ${GLM_DIR} PARENT_SCOPE)
