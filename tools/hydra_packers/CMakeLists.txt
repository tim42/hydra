

add_library(hydra_packers SHARED
    png_processor.cpp
    hsf_processor.cpp
    xz_processor.cpp

    raw_packer.cpp
    image_packer.cpp
    spirv_packer.cpp
    spirv_packer_big_dump_table.cpp

    process_helpers.cpp
)


target_compile_options(hydra_packers PRIVATE ${PROJECT_CXX_FLAGS})

target_include_directories(hydra_packers PRIVATE SYSTEM ${VULKAN_INCLUDE_DIR})

target_link_libraries(hydra_packers PUBLIC glm)
target_link_libraries(hydra_packers PUBLIC lodePNG)
target_link_libraries(hydra_packers PRIVATE ${GLSLANG_LIBRARIES})
#target_link_libraries(hydra_packers PUBLIC ntools)

target_include_directories(hydra_packers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hydra_packers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

foreach(target ntools fmt hydra)
    get_target_property(target_dirs ${target} INCLUDE_DIRECTORIES)
    target_include_directories(hydra_packers PUBLIC ${target_dirs})
endforeach()
