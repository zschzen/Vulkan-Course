#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>

#include "Utilities.h"



/**
 * @class VulkanRenderer
 * @brief The Vulkan Renderer
 */
class VulkanRenderer
{
public:

	VulkanRenderer();
	~VulkanRenderer();

	// ======================================================================================================================
	// ============================================ Vulkan Base Functions ===================================================
	// ======================================================================================================================

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

	// ======================================================================================================================
	// ============================================ Vulkan Variables ========================================================
	// ======================================================================================================================

	/** @brief The window to render to */
	GLFWwindow *m_window  {nullptr};

	/** @brief The debug messenger callback */
	VkDebugUtilsMessengerEXT m_debugMessenger {nullptr};

	/** @brief The Vulkan instance */
	VkInstance m_instance {nullptr};

	/** @brief The Vulkan surface */
	VkSurfaceKHR m_surface {nullptr};

	/**
	 * @struct MainDevice
	 * @brief Contains information about the Vulkan device and memory
	 */
	struct
	{
		VkPhysicalDevice  physicalDevice;    // Physical device (GPU) that Vulkan will use
		VkDevice          logicalDevice;     // Logical device (application's view of the physical device) that Vulkan will use
	} m_mainDevice {};

	// Handles to values. Don't actually hold values
	VkQueue m_graphicsQueue {};     // Queue that handles the passing of command buffers for rendering
	VkQueue m_presentationQueue {}; // Queue that handles presentation of images to the surface


	// ======================================================================================================================
	// ============================================ Vulkan Functions ========================================================
	// ======================================================================================================================

	// ++++++++++++++++++++++++++++++++++++++++++++++ Create Functions +++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief Create the Vulkan instance */
	void CreateInstance();

	/** @brief Create logical device */
	void CreateLogicalDevice();

	/** @brief Create the surface to render to */
	void CreateSurface();

	// ++++++++++++++++++++++++++++++++++++++++++++++ Get Functions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Get the required extensions for the Vulkan Instance
	 * @return The required extensions for the Vulkan Instance
	 */
	[[nodiscard]] std::vector<const char*> GetRequiredExtensions();

	/** @brief Get the required extensions for the Vulkan Instance */
	void GetPhysicalDevice();


	// ++++++++++++++++++++++++++++++++++++++++++++++ Support Functions +++++++++++++++++++++++++++++++++++++++++++++++++++

	// ++++++++++++++++++++++++++++++++++++++++++++++ Check Functions +++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Check if the requested validation layers are available
	 * @return True if the validation layers are available
	 */
	bool TryCheckValidationLayerSupport(std::string &outUnSupValLayer);

	/**
	 * @brief Checks if the Instance extensions are supported
	 * @param checkExtensions The extensions to check for support
	 * @param outUnSupExt The unsupported extension
	 * @return True if the instance extensions are supported
	 */
	bool TryCheckInstanceExtensionSupport(std::vector<const char *> *checkExtensions, std::string &outUnSupExt);

	/**
	 * @brief Checks if the Device extensions are supported
	 * @param device The physical device to check the extensions on
	 * @param outUnSupDevExt The unsupported device extension
	 * @return True if the device extensions are supported
	 */
	bool TryCheckDeviceExtensionSupport(VkPhysicalDevice device, std::string &outUnSupDevExt);

	/**
	 * @brief Checks if the given Vulkan physical device is suitable.
	 *
	 * @param device The Vulkan physical device to check.
	 * @return True if the device is suitable, false otherwise.
	 */
	bool CheckDeviceSuitable(VkPhysicalDevice device);

	// ++++++++++++++++++++++++++++++++++++++++++++++ Get Functions +++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Find the index of the queue family with the specified properties
	 *
	 * @param device The physical device to check the queue families on
	 * @return The indices of queue families that exist on the device
	 */
	queueFamilyIndices_t GetQueueFamilies(VkPhysicalDevice device);

	/**
	 * @brief Get the swap chain details
	 *
	 * @param device The physical device to check the swap chain details on
	 * @return The swap chain details
	 */
	swapChainDetails_t GetSwapChainDetails(VkPhysicalDevice device);
};

#endif //VULKANRENDERER_H
