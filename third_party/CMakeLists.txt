

# build GLFW (only what is realy needed)
set(GLFW_BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS FALSE CACHE BOOL "" FORCE)
set(GLFW_INSTALL FALSE CACHE BOOL "" FORCE)
set(GLFW_LIBRARY "glfw" PARENT_SCOPE)

add_subdirectory("./glfw")


# build LodePNG (create a static library)
include("./lodePNG.cmake")

# build imgui (create a static library)
include("./imgui.cmake")


# other easy to configure third party:
add_subdirectory("./fmt")
add_subdirectory("./glm")

if (${USE_TRACY})
  add_subdirectory("./tracy")
endif()

add_subdirectory("./ntools")

set(ENFIELD_IS_MASTER_PROJECT False)
add_subdirectory("./enfield")

