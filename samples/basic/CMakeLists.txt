##
## CMAKE file for neam/yaggler dev sample
##


# set the name of the sample
set(SAMPLE_NAME "basic")

add_executable(${SAMPLE_NAME} main.cpp)

target_compile_options(${SAMPLE_NAME} PRIVATE ${PROJECT_CXX_FLAGS})

target_include_directories(${SAMPLE_NAME} PRIVATE SYSTEM ${VULKAN_INCLUDE_DIR})
target_link_libraries(${SAMPLE_NAME} PUBLIC ${VULKAN_LIBRARY})
target_include_directories(${SAMPLE_NAME} PRIVATE SYSTEM ${LIBURING_INCLUDE_DIR})
target_link_libraries(${SAMPLE_NAME} PUBLIC ${LIBURING_LIBRARY})

target_include_directories(${SAMPLE_NAME} PRIVATE SYSTEM glm)
target_include_directories(${SAMPLE_NAME} PRIVATE SYSTEM lodePNG)
target_include_directories(${SAMPLE_NAME} PRIVATE SYSTEM imgui)
target_include_directories(${SAMPLE_NAME} PRIVATE SYSTEM glfw)
target_include_directories(${SAMPLE_NAME} PRIVATE SYSTEM fmt)
target_include_directories(${SAMPLE_NAME} PRIVATE ntools)
target_include_directories(${SAMPLE_NAME} PRIVATE hydra)
target_include_directories(${SAMPLE_NAME} PRIVATE enfield)


target_link_libraries(${SAMPLE_NAME} PUBLIC glm)
target_link_libraries(${SAMPLE_NAME} PUBLIC lodePNG)
target_link_libraries(${SAMPLE_NAME} PUBLIC imgui)
target_link_libraries(${SAMPLE_NAME} PUBLIC glfw)
target_link_libraries(${SAMPLE_NAME} PUBLIC ntools)
target_link_libraries(${SAMPLE_NAME} PUBLIC fmt)
target_link_libraries(${SAMPLE_NAME} PUBLIC hydra)
target_link_libraries(${SAMPLE_NAME} PUBLIC enfield)

