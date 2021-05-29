

# build GLFW (only what is realy needed)
set(GLFW_BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS FALSE CACHE BOOL "" FORCE)
set(GLFW_INSTALL FALSE CACHE BOOL "" FORCE)
set(GLFW_LIBRARY "glfw" PARENT_SCOPE)
get_filename_component(GLFW_DIR "./glfw/" ABSOLUTE)
set(GLFW_INCLUDE_DIR ${GLFW_DIR}/include PARENT_SCOPE)
add_subdirectory("./glfw")


# build LodePNG (create a static library)
set(LODEPNG_NAME "lodePNG")
set(LODEPNG_LIBRARY ${LODEPNG_NAME} PARENT_SCOPE)
get_filename_component(LODEPNG_DIR "./lodePNG/" ABSOLUTE)
set(LODEPNG_INCLUDE_DIR ${LODEPNG_DIR} PARENT_SCOPE)
set(LODEPNG_SOURCES "./lodePNG/lodepng.cpp")

include("./lodePNG.cmake")

# build imgui (create a static library)
set(IMGUI_NAME "imgui")
set(IMGUI_LIBRARY ${IMGUI_NAME} PARENT_SCOPE)
get_filename_component(IMGUI_DIR "./imgui/" ABSOLUTE)
set(IMGUI_INCLUDE_DIR ${IMGUI_DIR} PARENT_SCOPE)
set(IMGUI_BACKEND_INCLUDE_DIR ${IMGUI_DIR}/backend PARENT_SCOPE)

include("./imgui.cmake")

# GLM
get_filename_component(GLM_DIR "./glm/" ABSOLUTE)
set(GLM_INCLUDE_DIR ${GLM_DIR} PARENT_SCOPE)
