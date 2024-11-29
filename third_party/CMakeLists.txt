

# build GLFW (only what is realy needed)
set(GLFW_BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS FALSE CACHE BOOL "" FORCE)
set(GLFW_INSTALL FALSE CACHE BOOL "" FORCE)
set(GLFW_LIBRARY "glfw" PARENT_SCOPE)

# wayland doesn't allow window placement, but we need it
# wayland doesn't allow dynamically setting the window icon, but we need it
# (also, how can docking be implemented without window positioning?)
#
# For the window placemenet, there could be a solution to that, have a "force dock to" option in imgui windows,
# but this is definitely a workaround a not the way things should work
# There's no workaround for dynamically setting the window icon.
set(GLFW_BUILD_WAYLAND FALSE CACHE BOOL "" FORCE)

add_subdirectory("./glfw")

add_subdirectory("./meshoptimizer")


set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory("./assimp")


# build LodePNG (create a static library)
include("./lodePNG.cmake")

# build imgui (create a static library)
include("./imgui.cmake")


# other easy to configure third party:
add_subdirectory("./fmt")
add_subdirectory("./glm")

if (${USE_TRACY})
  set(TRACY_DELAYED_INIT TRUE CACHE BOOL "" FORCE)
  add_subdirectory("./tracy")
endif()

set(BUILD_NTOOLS_TESTS ON)
add_subdirectory("./ntools")

set(ENFIELD_IS_MASTER_PROJECT False)
add_subdirectory("./enfield")

