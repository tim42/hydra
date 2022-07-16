

add_library(hydra SHARED
    hydra.cpp
    resources/mimetype/mimetype.cpp
    resources/compressor.cpp
    resources/context.cpp
    resources/processor.cpp
    resources/packer.cpp
    resources/rel_db.cpp
    engine/engine.cpp
    engine/engine_module.cpp
    renderer/pass_manager.cpp
    #renderer/renderer_module.cpp

    # modules:
    engine/core_modules/io_module.cpp

    imgui/imgui_engine_module.cpp
    imgui/imgui_context.cpp
    imgui/generic_ui.cpp

    #glfw/glfw_engine_module.cpp
    glfw/glfw_window.cpp
)


target_compile_options(hydra PRIVATE ${PROJECT_CXX_FLAGS})

target_include_directories(hydra PRIVATE SYSTEM ${VULKAN_INCLUDE_DIR})
target_link_libraries(hydra PUBLIC ${VULKAN_LIBRARY})

target_include_directories(hydra PRIVATE SYSTEM ${LIBURING_INCLUDE_DIR})
target_link_libraries(hydra PUBLIC ${LIBURING_LIBRARY})

target_include_directories(hydra PRIVATE SYSTEM ${LIBMAGIC_INCLUDE_DIR})
target_link_libraries(hydra PRIVATE ${LIBMAGIC_LIBRARY})

target_link_libraries(hydra PUBLIC glm)
target_link_libraries(hydra PUBLIC X11)
#target_link_libraries(hydra PUBLIC ntools)

if (LIBLZMA_FOUND)
    target_include_directories(hydra PRIVATE SYSTEM ${LIBLZMA_INCLUDE_DIRS})
    target_link_libraries(hydra PUBLIC ${LIBLZMA_LIBRARIES})
endif()

target_include_directories(hydra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hydra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

foreach(target ntools fmt imgui glfw)
    get_target_property(target_dirs ${target} INCLUDE_DIRECTORIES)
    target_include_directories(hydra PUBLIC ${target_dirs})
    target_link_libraries(hydra INTERFACE ${target})
endforeach()
