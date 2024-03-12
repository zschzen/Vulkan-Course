#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>


#include <iostream>

int
main(int argc, char **argv)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    // Check how many extensions are supported
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    printf("Extension count: %d\n", extensionCount);

    // Test glm
    glm::mat4 matrix {1.0F};
    glm::vec4 vec {1.0F};
    auto test = matrix * vec; (void)test;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // if ESC is pressed, close the window
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, 1);
        }
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
