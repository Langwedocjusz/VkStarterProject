#include <GLFW/glfw3.h>

#include <cstdint>
#include <iostream>

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize glfw!";
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

    if (!window)
    {
        std::cerr << "Failed to create a window!";
        glfwTerminate();
        return -1;
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
