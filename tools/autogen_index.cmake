
# create a target for the imgui autogen:
add_custom_command( OUTPUT  ${CMAKE_BINARY_DIR}/hydra/embedded_index_generated.timestamp
                    BYPRODUCTS  ${CMAKE_BINARY_DIR}/hydra/embedded_index.hpp ${CMAKE_BINARY_DIR}/hydra/embedded_index.cpp
                    COMMAND embedded_index_builder ARGS --output=${CMAKE_BINARY_DIR}/hydra/embedded_index --namespace-name=neam::autogen::index --silent
                        shaders/engine/imgui/imgui.hsf
                        shaders/engine/generic/blur.hsf

                        fonts/NotoSans/NotoSans-Regular.ttf
                        fonts/NotoSans/NotoSans-Bold.ttf
                        fonts/NotoSans/NotoSans-Italic.ttf
                        fonts/NotoSans/NotoSans-BoldItalic.ttf

                        fonts/Hack/Hack-Regular.ttf
                        fonts/Hack/Hack-Bold.ttf
                        fonts/Hack/Hack-Italic.ttf
                        fonts/Hack/Hack-BoldItalic.ttf

                        images/hydra-logo.png.xz
                    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/hydra/embedded_index_generated.timestamp
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data/source/
                    MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/data/source/shaders/engine/imgui/imgui.hsf
                    DEPENDS embedded_index_builder
                    COMMENT "Generating hydra/embedded_index"
                    VERBATIM
)

add_custom_target(  generate_embedded_index
                    DEPENDS ${CMAKE_BINARY_DIR}/hydra/embedded_index.cpp
                            ${CMAKE_BINARY_DIR}/hydra/embedded_index.hpp
                            ${CMAKE_BINARY_DIR}/hydra/embedded_index_generated.timestamp
)
add_dependencies(generate_embedded_index embedded_index_builder)

add_library(embedded_index STATIC ${CMAKE_BINARY_DIR}/hydra/embedded_index.cpp)
target_include_directories(embedded_index PUBLIC ${CMAKE_BINARY_DIR})
add_dependencies(embedded_index generate_embedded_index)
