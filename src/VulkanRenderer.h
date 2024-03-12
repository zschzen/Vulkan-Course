#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "Utilities.h"

class VulkanRenderer
{
public:
	VulkanRenderer();

	~VulkanRenderer();

	/**
	 * @brief Initializes the Vulkan Renderer
	 *
	 * @param newWindow The window to render to
	 * @return 0 if the renderer was initialized successfully, 1 if it failed
	 */
	int Init(GLFWwindow *newWindow);

	/** @brief Cleans up the Vulkan Renderer */
	void Cleanup();

private:
	GLFWwindow *m_window{nullptr};

	// Vulkan Components
	VkInstance m_instance{NULL};

	/**
	 * @struct MainDevice
	 * @brief Contains information about the Vulkan device and memory
	 */
	struct
	{
		VkPhysicalDevice physicalDevice;    // Physical device (GPU) that Vulkan will use
		VkDevice logicalDevice;             // Logical device (application's view of the physical device) that Vulkan will use
	} mainDevice;

	VkQueue graphicsQueue;    // Queue that handles the passing of command buffers for rendering

	// ------------------------------------------
	// Vulkan Functions
	// ------------------------------------------

	// Create Functions

	/** @brief Create the Vulkan instance */
	void CreateInstance();

	/** @brief Create logical device */
	void CreateLogicalDevice();

	// Get Functions

	/** @brief Get the required extensions for the Vulkan Instance */
	void GetPhysicalDevice();

	// Support Functions
	// -- Check Functions

	/**
	 * @brief Checks if the Instance extensions are supported
	 * @param checkExtensions The extensions to check for support
	 * @param outUnSupExt The unsupported extension
	 * @return True if the instance extensions are supported
	 */
	static bool TryInstanceExtensionSupport(std::vector<const char *> *checkExtensions, std::string &outUnSupExt);

	/**
	 * @brief Checks if the given Vulkan physical device is suitable.
	 *
	 * @param device The Vulkan physical device to check.
	 * @return True if the device is suitable, false otherwise.
	 */
	static bool CheckDeviceSuitable(VkPhysicalDevice device);


	// -- Getter Functions

	/**
	 * @brief Find the index of the queue family with the specified properties
	 *
	 * @param device The physical device to check the queue families on
	 * @return The indices of queue families that exist on the device
	 */
	static queueFamilyIndices_t GetQueueFamilies(VkPhysicalDevice device);
};

#endif //VULKANRENDERER_H
