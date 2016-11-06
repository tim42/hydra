

if (UNIX)
  add_custom_target(BuildShaders ALL ./build-shaders.sh
                    WORKING_DIRECTORY ./
                    COMMENT "Compiling shaders into SPIRV")
endif()
