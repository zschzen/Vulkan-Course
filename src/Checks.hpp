#ifndef CHECKS_HPP
#define CHECKS_HPP

#include <vulkan/vulkan_core.h>

#include <stdexcept>
#include <cstdio>

/**
 * @brief Convert a VkResult to a string
 *
 * @param err The error to convert
 * @return The string representation of the error
 */
[[nodiscard]]
static const char*
VkResultToString(VkResult err)
{
	switch(err)
	{
		case VK_SUCCESS:                        return "VK_SUCCESS";
		case VK_NOT_READY:                      return "VK_NOT_READY";
		case VK_TIMEOUT:                        return "VK_TIMEOUT";
		case VK_EVENT_SET:                      return "VK_EVENT_SET";
		case VK_EVENT_RESET:                    return "VK_EVENT_RESET";
		case VK_INCOMPLETE:                     return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:       return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:     return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:    return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:              return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:        return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:        return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:    return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:      return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:      return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:         return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:     return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_SURFACE_LOST_KHR:         return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_SUBOPTIMAL_KHR:                 return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:          return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:    return "VK_ERROR_VALIDATION_FAILED_EXT";
		default: return "Unknown error";
	}
}

/// Check if a Vulkan function call was successful
#define VK_CHECK(x, msg)             \
do                                   \
{                                    \
	VkResult err = x;                  \
	if (err)                           \
	{                                  \
		const char* errStr = VkResultToString(err); \
		fprintf(stderr, "!------------ Vulkan Error ------------!\n"); \
		fprintf(stderr, "%s @ %s:%i\n", #x, __FILE__, __LINE__); \
		fprintf(stderr, "Error: %s\n", errStr); \
		throw std::runtime_error(msg); \
	}                                \
} while (0)


#endif // CHECKS_HPP
