#include <chrono>
#include <thread>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"


//
GLFWwindow *window;
VulkanRenderer vulkanRenderer;



void
InitWindow(const std::string &wName = "Vulkan Window", const int width = 800, const int height = 600)
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
	InitWindow("Vulkan Window", 800, 600);

	// Try to create Vulkan Instance
	if (vulkanRenderer.Init(window) != EXIT_SUCCESS) return EXIT_FAILURE;

	// Main loop
	{
		// model
		float angle = 0.0f;

		// Time
		int maxFPS = 60;
		double deltaTime = 0.0f;
		double lastTime = 0.0f;
		double currentTime = 0.0f;
		double desiredFrameTime = 1.0f / maxFPS;

		bool isRunning = true;
		while (isRunning && !glfwWindowShouldClose(window))
		{
			// ------------------------------------------- Time -------------------------------------------
			currentTime = glfwGetTime();
			deltaTime = currentTime - lastTime;

			// Limit FPS
			if (deltaTime < desiredFrameTime)
			{
				int sleepTime = static_cast<int>((desiredFrameTime - deltaTime) * 1000);
				if (sleepTime > 0)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
				}
			}

			// Update the previous frame time
			lastTime = currentTime;

			// update glfw title
			{
				std::string fps = std::format("{:.2f}", 1.0 / deltaTime);
				glfwSetWindowTitle(window, ("Vulkan Window - FPS: " + fps).c_str());
			}

			// ------------------------------------------- Input -------------------------------------------
			glfwPollEvents();

			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			{
				isRunning = false;
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			}

			// ------------------------------------------- Update -------------------------------------------

			// Rotate model based on deltaTime
			angle += std::fmod(45.0f * static_cast<float>(deltaTime), 360.0f);
			glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
			vulkanRenderer.UpdateModel(rotation);

			// ------------------------------------------- Render -------------------------------------------
			vulkanRenderer.Draw();
		}
	}

	// Clean up
	vulkanRenderer.Cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
