
set(IMGUI_ADD_SOURCES )
if (${FREETYPE_FOUND})
    set(IMGUI_ADD_SOURCES ${IMGUI_ADD_SOURCES} imgui/misc/freetype/imgui_freetype.cpp)
endif()

add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp

    imgui/imgui.h
    imgui/imgui_internal.h
    imgui/imconfig.h

    ${IMGUI_ADD_SOURCES}
)
target_include_directories(imgui PUBLIC imgui_wrapper/ imgui/)
if (${FREETYPE_FOUND})
    target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE=1)
    target_include_directories(imgui PUBLIC imgui/misc/freetype/)
    target_include_directories(imgui PRIVATE ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(imgui PUBLIC ${FREETYPE_LIBRARIES})
endif()
