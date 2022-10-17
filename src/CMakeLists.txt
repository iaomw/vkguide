
file(GLOB THE_SOURCES CONFIGURE_DEPENDS "*.h" "*.c" "*.cpp")
add_executable(vulkan_guide ${THE_SOURCES})
#target_sources(vulkan_guide PUBLIC main.cpp)

add_dependencies(vulkan_guide Shaders)

# set_property(TARGET vulkan_guide PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:vulkan_guide>")

target_include_directories(vulkan_guide PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(vulkan_guide 
                vkbootstrap vma glm 
                spirv_reflect
                tinyobjloader  
                imgui stb_image)

target_link_libraries(vulkan_guide Vulkan::Vulkan sdl2)

# find_package(glm CONFIG REQUIRED)
# target_link_libraries(vulkan_guide glm::glm)

# find_package(fmt CONFIG REQUIRED)
# target_link_libraries(vulkan_guide fmt::fmt)

# find_package(lz4 CONFIG REQUIRED)
# target_link_libraries(vulkan_guide lz4::lz4)

# find_package(nlohmann_json CONFIG REQUIRED)
# target_link_libraries(vulkan_guide nlohmann_json::nlohmann_json)

if(HAS_CLANGD)
    add_custom_command(TARGET vulkan_guide POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink #copy_directory
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
    )
endif()