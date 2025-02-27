

add_library(hydra SHARED
    hydra.cpp

    vulkan/instance.cpp
    vulkan/submit_info.cpp

    resources/mimetype/mimetype.cpp
    resources/compressor.cpp
    resources/index.cpp
    resources/context.cpp
    resources/processor.cpp
    resources/packer.cpp
    resources/rel_db.cpp
    resources/metadata.cpp

    engine/core_context.cpp
    engine/engine.cpp
    engine/engine_module.cpp
    engine/conf/context.cpp

    renderer/renderer_engine_module.cpp
    renderer/resources/texture_manager.cpp
    renderer/resources/mesh_manager.cpp
    renderer/ecs/gpu_task_producer.cpp
    renderer/ecs/gpu_tasks_order.cpp

    utilities/allocator/block_allocator.cpp
    utilities/allocator/scoped_pool.cpp
    utilities/allocator/transient_pool.cpp
    utilities/allocator/allocator_set.cpp
    utilities/allocator/allocator_scope.cpp
    utilities/command_pool_manager.cpp
    utilities/deferred_queue_execution.cpp
    utilities/deferred_fence_execution.cpp
    utilities/pipeline_manager.cpp
    utilities/pipeline_render_state.cpp
    utilities/transfer.cpp
    utilities/transfer_context.cpp
    utilities/descriptor_allocator.cpp
    utilities/shader_gen/descriptor_sets_runtime.cpp

    # modules:
    engine/core_modules/core_module.cpp
    engine/core_modules/io_module.cpp

    imgui/imgui_engine_module.cpp
    imgui/imgui_context.cpp
    imgui/generic_ui.cpp
    imgui/ui_elements.cpp
    imgui/generic_ui_elements/generic_ui_elements.cpp

    glfw/glfw_engine_module.cpp
    glfw/glfw_events.cpp
    glfw/glfw_window.cpp

    renderer/generic_shaders/downsample.cpp
    renderer/generic_shaders/blur.cpp

    ecs/universe.cpp
    ecs/hierarchy.cpp
    ecs/transform.cpp
)


target_compile_options(hydra PRIVATE ${PROJECT_CXX_FLAGS})

target_include_directories(hydra PRIVATE SYSTEM ${VULKAN_INCLUDE_DIR})
target_include_directories(hydra PUBLIC ${CMAKE_BINARY_DIR})
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

foreach(target ntools fmt imgui glfw enfield)
    get_target_property(target_dirs ${target} INCLUDE_DIRECTORIES)
    target_include_directories(hydra PUBLIC ${target_dirs})
    target_link_libraries(hydra INTERFACE ${target})
endforeach()

##
## Shader struct
##

add_library("shader-structs" OBJECT shader_structs.cpp utilities/shader_gen/block.cpp utilities/shader_gen/descriptor_sets_generator.cpp)
target_compile_definitions("shader-structs" PUBLIC N_HYDRA_SHADERS_ALLOW_GENERATION=true)
target_include_directories("shader-structs" PUBLIC hydra)
target_link_libraries("shader-structs" PUBLIC hydra)
target_compile_options("shader-structs" PUBLIC ${PROJECT_CXX_FLAGS})
