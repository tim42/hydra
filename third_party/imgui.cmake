
add_library(imgui STATIC
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_glfw.h

    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp

    imgui/imgui.h
    imgui/imgui_internal.h
    imgui/imconfig.h
)

target_include_directories(imgui PRIVATE glfw)
target_link_libraries(imgui PUBLIC glfw)
target_include_directories(imgui PUBLIC imgui/backends imgui/)
