#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#define DEBUG

#include "VulkanRenderer.h"

GLFWwindow *window;
VulkanRenderer vulkanRenderer;

void
InitWindow(const std::string &wName = "Test Window", const int width = 800, const int height = 600)
{
	if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");

	// Set GLFW to not create an OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int
main()
{
	// Window setup
	InitWindow("Test Window", 800, 600);

	// Try to create Vulkan Instance
	if (vulkanRenderer.Init(window) != EXIT_SUCCESS) return EXIT_FAILURE;

	// Main loop
	bool isRunning = true;
	while (isRunning && !glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			isRunning = false;
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}

	// Clean up
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
