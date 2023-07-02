##
## CMake file for tools
##

add_subdirectory(./hydra_packers)
add_subdirectory(./resource_server)
add_subdirectory(./embedded_index_builder)

include(autogen_index.cmake)

add_subdirectory(./config_editor)
