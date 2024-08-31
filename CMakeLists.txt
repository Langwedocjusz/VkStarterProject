cmake_minimum_required(VERSION 3.30.2)

project(VkStarterProject)

include(cmake/utils.cmake)

add_executable(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

enable_warnings(${PROJECT_NAME})

#To enable usage of clang-tidy:
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)

#Get dependencies:
find_package(Vulkan REQUIRED)

add_subdirectory(vendor/glfw)

#Link dependencies:
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

#Dependency-specific definitions:
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_VULKAN")