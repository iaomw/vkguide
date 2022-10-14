
file(GLOB THE_SOURCES CONFIGURE_DEPENDS "*.h" "*.c" "*.cpp")
add_executable(vulkan_guide ${THE_SOURCES})
#target_sources(vulkan_guide PUBLIC main.cpp)


# set_property(TARGET vulkan_guide PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkan_guide>")

target_include_directories(vulkan_guide PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkan_guide 
                vkbootstrap vma glm 
                spirv_reflect
                tinyobjloader  
                imgui stb_image)

target_link_libraries(vulkan_guide Vulkan::Vulkan sdl2)

add_dependencies(vulkan_guide Shaders)
