#ifndef VULKAN_VALIDATION_H
#define VULKAN_VALIDATION_H

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef NDEBUG
	#define ENABLE_VALIDATION_LAYERS false
#else
	#define ENABLE_VALIDATION_LAYERS true
#endif



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
// ============================================ Debug Callback ==========================================================
// ======================================================================================================================

/** @brief The debug callback function */
static VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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


// ======================================================================================================================
// ============================================ Debug Messenger =========================================================
// ======================================================================================================================

/** @brief Populates the debug messenger create info */
static void
PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
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

/** @brief Creates the debug messenger */
static VkResult
CreateDebugUtilsMessengerEXT(VkInstance instance,
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


// ======================================================================================================================
// ============================================ Destroy Debug Messenger =================================================
// ======================================================================================================================

/** @brief Destroys the debug messenger */
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
										  const VkAllocationCallbacks *pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

#endif //VULKAN_VALIDATION_H
