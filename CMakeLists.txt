cmake_minimum_required(VERSION 3.30.2)

project(VkStarterProject)

include(cmake/utils.cmake)

if(MSVC)
    msvc_enable_multiprocess_global()
endif()

add_executable(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

enable_warnings(${PROJECT_NAME})

if(MSVC)
    msvc_startup_project(${PROJECT_NAME})
    msvc_working_directory(${PROJECT_NAME} ${CMAKE_SOURCE_DIR})
endif()

#To enable usage of clang-tidy:
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

#Sources
target_sources(${PROJECT_NAME}
    PRIVATE
        #Also listing headers to make them visible in IDEs
        src/Application.h
        src/Application.cpp
        src/ImGuiContext.h
        src/ImGuiContext.cpp
        src/main.cpp
        src/Renderers/HelloTriangle.h
        src/Renderers/HelloTriangle.cpp
        src/Renderers/MainMenu.h
        src/Renderers/MainMenu.cpp
        src/Renderers/RendererBase.h
        src/Renderers/RendererBase.cpp
        src/Renderers/TexturedQuad.h
        src/Renderers/TexturedQuad.cpp
        src/SystemWindow.h
        src/SystemWindow.cpp
        src/Utils.h
        src/Utils.cpp
        src/VulkanContext.h
        src/VulkanContext.cpp
)

#Implementation files for imgui
file(GLOB imgui_impl vendor/imgui/backends/imgui_impl_vulkan.cpp vendor/imgui/backends/imgui_impl_glfw.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${imgui_impl})

#Include directories
target_include_directories(${PROJECT_NAME} PRIVATE src src/Renderers)

#For filters in IDEs like Visual studio:
source_group(src REGULAR_EXPRESSION "src/*")
source_group(src/Renderers REGULAR_EXPRESSION "src/Renderers/*")
source_group(imgui_impl FILES ${imgui_impl})

#Get dependencies:
find_package(Vulkan REQUIRED)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/vk-bootstrap)
add_subdirectory(vendor/imgui)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/stb_image)

#Link dependencies:
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE vk-bootstrap::vk-bootstrap)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_link_libraries(${PROJECT_NAME} PRIVATE stb_image)

#Dependency-specific definitions:
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_VULKAN")