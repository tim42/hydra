

add_custom_target(copy-data-dir ALL
                  cmake -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data
                  COMMENT "Copy the data dir to the build directory")
if (UNIX)
  add_custom_target(build-shaders ALL
                    ./build-shaders.sh
                    WORKING_DIRECTORY ./data/
                    COMMENT "Compiling shaders into SPIRV")
  add_dependencies(build-shaders copy-data-dir)
endif()
