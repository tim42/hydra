

add_library(hydra_packers SHARED
    png_processor.cpp
    hsf_processor.cpp
    xz_processor.cpp
    obj_processor.cpp

    raw_packer.cpp
    image_packer.cpp
    static_mesh_packer.cpp
    spirv_packer.cpp
    spirv_packer_big_dump_table.cpp


    process_helpers.cpp
)


target_compile_options(hydra_packers PRIVATE ${PROJECT_CXX_FLAGS})

target_include_directories(hydra_packers PRIVATE SYSTEM ${VULKAN_INCLUDE_DIR})

target_link_libraries(hydra_packers PRIVATE glm)
target_link_libraries(hydra_packers PRIVATE lodePNG)
target_link_libraries(hydra_packers PRIVATE meshoptimizer)
target_link_libraries(hydra_packers PRIVATE assimp)
target_link_libraries(hydra_packers PRIVATE ${GLSLANG_LIBRARIES})
#target_link_libraries(hydra_packers PUBLIC ntools)

target_include_directories(hydra_packers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hydra_packers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

foreach(target ntools fmt hydra)
    get_target_property(target_dirs ${target} INCLUDE_DIRECTORIES)
    target_include_directories(hydra_packers PUBLIC ${target_dirs})
endforeach()


# link with shader structs libraries:
function (_get_all_cmake_targets out_var current_dir)
    get_property(targets DIRECTORY ${current_dir} PROPERTY BUILDSYSTEM_TARGETS)
    get_property(subdirs DIRECTORY ${current_dir} PROPERTY SUBDIRECTORIES)

    foreach(subdir ${subdirs})
        _get_all_cmake_targets(subdir_targets ${subdir})
        list(APPEND targets ${subdir_targets})
    endforeach()

    set(${out_var} ${targets} PARENT_SCOPE)
endfunction()
set(all_targets "")
_get_all_cmake_targets(all_targets ${CMAKE_SOURCE_DIR})
target_link_libraries(hydra_packers PUBLIC shader-structs)
# target_compile_definitions(hydra_packers PUBLIC N_HYDRA_SHADERS_ALLOW_GENERATION=true)
foreach(target ${all_targets})
  if (target MATCHES "^shader-structs.+")
    target_link_libraries(hydra_packers PRIVATE ${target})
  endif()
endforeach()
