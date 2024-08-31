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

target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)

#For filters in IDEs like Visual studio:
source_group(src REGULAR_EXPRESSION "src/*")

#Get dependencies:
find_package(Vulkan REQUIRED)

add_subdirectory(vendor/glfw)

#Link dependencies:
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

#Dependency-specific definitions:
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_VULKAN")