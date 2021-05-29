
set(IMGUI_SOURCES
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp)
#target_include_directories(${IMGUI_NAME} PUBLIC ${IMGUI_INCLUDE_DIR} PUBLIC ${IMGUI_BACKEND_INCLUDE_DIR})
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ${GLFW_DIR}/include)
add_library(${IMGUI_NAME} STATIC ${IMGUI_SOURCES})
