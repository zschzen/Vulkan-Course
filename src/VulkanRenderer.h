#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#ifdef NDEBUG
	#define ENABLE_VALIDATION_LAYERS false
#else
	#define ENABLE_VALIDATION_LAYERS true
#endif

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
	// ============================================ Vulkan Constants ========================================================
	// ======================================================================================================================

	/** @brief The validation layers to enable */
	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation",
		//"VK_LAYER_LUNARG_monitor",
		//"VK_LAYER_LUNARG_api_dump",
	};

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

	/** @brief Create the debug messenger to enable validation layers */
	void CreateDebugMessenger();

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


	// ++++++++++++++++++++++++++++++++++++++++++++++ Debug Functions +++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Create the debug messenger to enable validation layers
	 * @param instance The Vulkan instance to associate the debug messenger with
	 * @param pCreateInfo The debug messenger create info
	 * @param pAllocator The allocator to use
	 * @param pDebugMessenger The debug messenger to create
	 * @return The result of the debug messenger creation
	 */
	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
												 const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	/** @brief Populate the debug messenger create info */
	static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

	/** @brief The debug callback function */
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	/** @brief Cleanup the debug messenger */
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
											  const VkAllocationCallbacks* pAllocator);
};





// ======================================================================================================================
// ============================================ Inline Function Definitions =============================================
// ======================================================================================================================


inline VkResult
VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance,
											 const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
											 const VkAllocationCallbacks *pAllocator,
											 VkDebugUtilsMessengerEXT *pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

// TODO: Implement macros for user choosing debug messages level
inline void
VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
	createInfo =
	{
		// The type of this structure
		.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

		// The message severity is a bitmask of VkDebugUtilsMessageSeverityFlagBitsEXT specifying which severity of
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
	                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

		// The message type is a bitmask of VkDebugUtilsMessageTypeFlagBitsEXT specifying which types of
		.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
	                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,

		// Set the callback function
		.pfnUserCallback = DebugCallback,
		// The user data to pass to the callback. Optional
		.pUserData       = nullptr
	};
}

// TODO: Implement spdlog for logging
// TIP: The VKAPI_ATTR and VKAPI_CALL ensure that the function has the right signature for Vulkan to call it.
inline VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanRenderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
							  VkDebugUtilsMessageTypeFlagsEXT messageType,
							  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
							  void* pUserData)
{
	// Unused parameters
	(void) messageType; (void) pUserData;

	// Get the severity(Verbose, Info, Warning, Error)
    std::string severity {};
    switch (messageSeverity)
	{
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT  : severity = "Verbose";  break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT     : severity = "Info";     break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT  : severity = "Warning";  break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT    : severity = "Error";    break;
        default: severity = "Unknown"; break;
    }

	// Print the message
	std::cerr << "[Validation Layer] [" << severity << "]: " << pCallbackData->pMessage << std::endl;

	// Return false to indicate that the Vulkan call should be aborted
    return VK_FALSE;
}

inline void
VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
											  const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

#endif //VULKANRENDERER_H
