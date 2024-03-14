#include "VulkanRenderer.h"

#include "VulkanValidation.h"
#include <cstring>


VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer()
{
	Cleanup();
}



int
VulkanRenderer::Init(GLFWwindow *newWindow)
{
	m_window = newWindow;

	try
	{
		CreateInstance();
		CreateDebugMessenger();
		CreateSurface();
		GetPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
	}
	catch (const std::runtime_error &e)
	{
		fprintf(stderr, "[ERROR] %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void
VulkanRenderer::Cleanup()
{
	// Destroy debug callback
	if (ENABLE_VALIDATION_LAYERS)
	{
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		m_debugMessenger = nullptr;
	}

	// Destroy the image views and the swap chain
	for (const auto &image : m_swapChainImages)
	{
		vkDestroyImageView(m_mainDevice.logicalDevice, image.imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_mainDevice.logicalDevice, m_swapchain, nullptr);
	m_swapChainImages.clear();
	m_swapchain = nullptr;

	// Destroy the surface
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	m_surface = nullptr;

	// Destroy the logical device
	vkDestroyDevice(m_mainDevice.logicalDevice, nullptr);
	m_mainDevice = {nullptr};

	// Destroy the Vulkan Instance
	vkDestroyInstance(m_instance, nullptr);
	m_instance = nullptr;
}

void
VulkanRenderer::CreateInstance()
{
	// Check if validation layers are available
	{
		std::string unSupValLayer{};
		if (ENABLE_VALIDATION_LAYERS && !TryCheckValidationLayerSupport(unSupValLayer))
		{
			throw std::runtime_error("Validation layer not supported: " + unSupValLayer);
		}
	}

	// Information about the application.
	// Debugging purposes. Used for developer convenience.
	VkApplicationInfo appInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext              = nullptr,
		.pApplicationName   = "Vulkan App",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName        = "No Engine",
		.engineVersion      = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion         = VK_API_VERSION_1_0
	};

	// Instance extensions
	std::vector<const char *> instanceExtensions = GetRequiredExtensions();

	// Creation information for a Vulkan Instance
	VkInstanceCreateInfo createInfo =
	{
		.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,                // The type of this structure
		.pNext                   = nullptr,                                               // Pointer to extension information
		.flags                   = 0,                                                     // Reserved for future use
		.pApplicationInfo        = &appInfo,                                              // Information about the application
		.enabledExtensionCount   = static_cast<uint32_t>(instanceExtensions.size()),      // Number of extensions to enable
		.ppEnabledExtensionNames = instanceExtensions.data()                              // Names of the extensions to enable
	};

	// Validation layer information
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (ENABLE_VALIDATION_LAYERS)
	{
		createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// Create the Vulkan Instance
	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan Instance");
	}
}

void
VulkanRenderer::CreateLogicalDevice()
{
	// Get queue family indices for the physical device
	queueFamilyIndices_t indices = GetQueueFamilies(m_mainDevice.physicalDevice);

	// Vector for queue creation information, and set for family indices
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	// Queues the logical device
	float queuePriority = 1.0F;
	for (int queueFamilyIndex: queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo =
		{
			.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = static_cast<uint32_t>(queueFamilyIndex),
			.queueCount       = 1,
			.pQueuePriorities = &queuePriority
		};

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Logical device creation
	// TIP: Device is Logical device. PhysicalDevice is Physical Device

	VkPhysicalDeviceFeatures physicalDeviceFeatures {};

	VkDeviceCreateInfo deviceCreateInfo =
	{
		.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount     = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos        = queueCreateInfos.data(),
		.enabledExtensionCount    = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames  = deviceExtensions.data(),
		.pEnabledFeatures         = &physicalDeviceFeatures
	};

	if (vkCreateDevice(m_mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &m_mainDevice.logicalDevice) != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a logical device");
	}

	// Queues are created at the same time as the device
	vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.presentationFamily, 0, &m_presentationQueue);
}

void
VulkanRenderer::CreateDebugMessenger()
{
	if (!ENABLE_VALIDATION_LAYERS) return;

	// Create the debug messenger
	VkDebugUtilsMessengerCreateInfoEXT createInfo {};
	PopulateDebugMessengerCreateInfo(createInfo);

	// Create the debug messenger
	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to set up debug messenger");
	}
}

void
VulkanRenderer::CreateSurface()
{
	VkResult result = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);

	if (result != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Surface!");
	}
}

void
VulkanRenderer::CreateSwapChain()
{
	// Best swap chain settings
	swapChainDetails_t swapChainDetails = GetSwapChainDetails(m_mainDevice.physicalDevice);

	// Best/Optimal values for swap chain
	VkSurfaceFormatKHR surfaceFormat    = ChooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentationMode   = ChooseBestPresentationMode(swapChainDetails.presentationModes);
	VkExtent2D extent                   = ChooseSwapExtent(swapChainDetails.surfaceCapabilities);

	// Swap chain image count (+1 to have a triple buffered system)
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

	// If imageCount higher than the max, then clamp it
	// If 0, then limitless
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0
		&& swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	// Creation information for the swap chain
	VkSwapchainCreateInfoKHR swapChainCreateInfo =
	{
		.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface          = m_surface,                                              // Swapchain surface
		.minImageCount    = imageCount,                                             // Minimum images in swap chain
		.imageFormat      = surfaceFormat.format,                                   // Swapchain image format
		.imageColorSpace  = surfaceFormat.colorSpace,                               // Swapchain color space
		.imageExtent      = extent,                                                 // Swapchain image size
		.imageArrayLayers = 1,                                                      // Number of layers for each image in chain
		.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,                    // What attachment images will be used as (e.g. color, depth, stencil)
		.preTransform     = swapChainDetails.surfaceCapabilities.currentTransform,  // Transformations to be applied to images in chain
		.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,                      // How to handle blending images with external graphics
		.presentMode      = presentationMode,                                       // How images should be presented to the screen
		.clipped          = VK_TRUE,                                                // Whether to clip parts of images not in view (e.g. behind another window, off screen, etc)
		.oldSwapchain     = VK_NULL_HANDLE                                          // Swapchain to replace, used for resizing the window
	};

	// Get the queue family indices
	queueFamilyIndices_t indices = GetQueueFamilies(m_mainDevice.physicalDevice);

	// If the graphics and presentation families are different, then swap chain images can be used across families
	if (indices.graphicsFamily != indices.presentationFamily)
	{
		uint32_t queueFamilyIndices[2] =
		{
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily
		};

		swapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;       // Image share handling
		swapChainCreateInfo.queueFamilyIndexCount = 2;                                // Number of queues to share images between
		swapChainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;               // Array of queues to share between
	}
	else
	{
		swapChainCreateInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;         // Single queue family can access images
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices   = nullptr;
	}

	// Create Swapchain
	if (vkCreateSwapchainKHR(m_mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &m_swapchain) != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Swapchain!");
	}

	// Store for later reference
	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;

	// Get swap chain images count
	uint32_t swapChainImageCount;
	vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapchain, &swapChainImageCount, nullptr);

	// Get the swap chain images
	std::vector<VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapchain, &swapChainImageCount, images.data());

	for (const auto &image : images)
	{
		// Store image handle
		swapchainImage_t swapChainImage =
		{
			.image     = image,
			.imageView = CreateImageView(image, m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT)
		};

		// Store image and image view
		m_swapChainImages.push_back(swapChainImage);
	}
}

std::vector<const char*>
VulkanRenderer::GetRequiredExtensions()
{
	// GLFW Extensions
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Create a vector to hold the extensions
	std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	// Check if the instance extensions are supported
	{
		std::string unSupExt{};
		if (!TryCheckInstanceExtensionSupport(&extensions, unSupExt))
		{
			throw std::runtime_error("VkInstance does not support required extension: " + unSupExt);
		}
	}

	// Enable the debug extension if validation layers are enabled
	if (ENABLE_VALIDATION_LAYERS)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

#ifndef NDEBUG
	// Print the extensions
	fprintf(stdout, "[INFO] Required Extensions:\n");
	for (const auto &extension : extensions)
	{
		fprintf(stdout, "\t%s\n", extension);
	}
#endif

	return extensions;
}

void
VulkanRenderer::GetPhysicalDevice()
{
	// Enumerate Physical devices the vkInstance can access
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	// If no devices available, then none support Vulkan!
	if (deviceCount <= 0)
	{
		throw std::runtime_error("Can't find GPUs that support Vulkan Instance");
	}

	// Get the physical devices
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, deviceList.data());

	for (const auto &device : deviceList)
	{
		if (!CheckDeviceSuitable(device)) continue;

		m_mainDevice.physicalDevice = device;
		break;
	}
}

bool
VulkanRenderer::TryCheckValidationLayerSupport(std::string &outUnSupValLayer)
{
	// Number of validation layers to check
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	// Available validation layers
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// Check if the required validation layers are available
	for (const auto &layerName : validationLayers)
	{
		bool bLayerFound = false;
		for (const auto &layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				bLayerFound = true;
				break;
			}
		}

		// Unsupported validation layer found
		if (!bLayerFound)
		{
			outUnSupValLayer = std::string(layerName);
			return false;
		}
	}

	// All required validation layers are supported
	return true;
}

bool
VulkanRenderer::TryCheckInstanceExtensionSupport(std::vector<const char *> *checkExtensions, std::string &outUnSupExt)
{
	// Number of extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// List of VkExtensionProperties
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// Check if the required extensions are in the available extensions
	for (const auto &checkExtension : *checkExtensions)
	{
		bool bHasExtension = false;
		for (const auto &extension : extensions)
		{
			// Determine if the extension is supported
			if (strcmp(checkExtension, extension.extensionName) == 0)
			{
				bHasExtension = true;
				break;
			}
		}

		// Unsupported extension found
		if (!bHasExtension)
		{
			outUnSupExt = std::string(checkExtension);
			return false;
		}
	}

	// All required extensions are supported
	return true;
}

bool
VulkanRenderer::TryCheckDeviceExtensionSupport(VkPhysicalDevice device, std::string &outUnSupDevExt)
{
	// Get device extension count
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// If no extensions found, return failure
	if (extensionCount <= 0) return false;

	// Populate extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	// Check for extension
	for (const auto &deviceExtension : deviceExtensions)
	{
		bool bHasExtension = false;
		for (const auto &extension : extensions)
		{
			if (strcmp(deviceExtension, extension.extensionName) == 0)
			{
				bHasExtension = true;
				break;
			}
		}

		// Unsupported extension found
		if (!bHasExtension)
		{
			outUnSupDevExt = std::string(deviceExtension);
			return false;
		}
	}

	// All required extensions are supported
	return true;
}

bool
VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice device)
{
	// Information about the device itself (ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// Check if the device supports the required extensions
	std::string unSupDevExt{};
	if (!TryCheckDeviceExtensionSupport(device, unSupDevExt))
	{
		fprintf(stderr, "[ERROR] Device does not support required extension: %s\n", unSupDevExt.c_str());
		return false;
	}

	// Check if the device supports the required queue families
	queueFamilyIndices_t indices = GetQueueFamilies(device);
	if (!indices.IsValid())
	{
		fprintf(stderr, "[ERROR] Device does not support required queue families\n");
		return false;
	}

	// Check if the device supports the swap chain extension
	bool bSwapChainValid = false;
	if (indices.IsValid())
	{
		swapChainDetails_t swapChainDetails = GetSwapChainDetails(device);
		bSwapChainValid = !swapChainDetails.formats.empty() && !swapChainDetails.presentationModes.empty();
	}

	// Check if the device is suitable
	return indices.IsValid() &&
		   bSwapChainValid;
		  //deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		  //deviceFeatures.geometryShader
}

queueFamilyIndices_t
VulkanRenderer::GetQueueFamilies(VkPhysicalDevice device)
{
	queueFamilyIndices_t indices {};

	// Number of queue families
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	// Get the queue family properties
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	// Each queue family may support multiple types of operations
	for (int i = 0; i < queueFamilyCount; ++i)
	{
		const VkQueueFamilyProperties &queueFamily = queueFamilies[i];

		// Check if the queue family has at least 1 of the required types of queues
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		// Check queueFamily presentation
		// Is the surface presentation supported on this device for this queue family?
		VkBool32 vbPresentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &vbPresentationSupport);
		if (queueFamily.queueCount > 0 && vbPresentationSupport)
		{
			indices.presentationFamily = i;
		}

		// Check if the queue family is valid
		if (indices.IsValid()) break;
	}

	return indices;
}

swapChainDetails_t
VulkanRenderer::GetSwapChainDetails(VkPhysicalDevice device)
{
	swapChainDetails_t swapChainDetails {};

	// -- CAPABILITIES --
	// Get the surface capabilities for the given surface on the given physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &swapChainDetails.surfaceCapabilities);

	// -- SURFACE FORMATS --
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	// If formats returned, get the list of formats
	if (formatCount != 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, swapChainDetails.formats.data());
	}

	// -- PRESENTATION MODES --
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount, nullptr);

	// If presentation modes returned, get the list of presentation modes
	if (presentationCount != 0)
	{
		swapChainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount,
												  swapChainDetails.presentationModes.data());
	}

	return swapChainDetails;
}

VkSurfaceFormatKHR
VulkanRenderer::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &InFormats)
{
	// If the surface has no preferred format, then return the first format available
	if (InFormats.size() == 1 && InFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// If the surface has a preferred format, then return it
	for (const auto &format : InFormats)
	{
		if ( ( format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM ) &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
		{
			return format;
		}
	}

	// First available
	return InFormats[0];
}

VkPresentModeKHR
VulkanRenderer::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> &InPresentationModes)
{
	// Look for mailbox presentation mode
	// Mailbox is the lowest latency V-Sync enabled mode (something like triple buffering)
	for (const auto &presentationMode : InPresentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}

	// If not found, use FIFO as it is always available
	// FIFO is the only mode that is guaranteed to be available
	// 		is the equivalent of V-Sync (something like double buffering)
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &InSurfaceCapabilities)
{
	// If the surface size is defined, then use it
	if (InSurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return InSurfaceCapabilities.currentExtent;
	}
	else
	{
		// Set the size of the window to the size of the surface
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);

		VkExtent2D actualExtent =
		{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		// Clamp the value to the available minimum and maximum extents
		actualExtent.width  = std::max(InSurfaceCapabilities.minImageExtent.width,
									   std::min(InSurfaceCapabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(InSurfaceCapabilities.minImageExtent.height,
									   std::min(InSurfaceCapabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VkImageView
VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	// Create an image view for the image
	VkImageViewCreateInfo viewInfo =
	{
		.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image            = image,                                        // Image to create view for
		.viewType         = VK_IMAGE_VIEW_TYPE_2D,                        // Type of image (1D, 2D, 3D, Cube, etc)
		.format           = format,                                       // Format of the image data
		.components       =                                               // Allows for swizzling of the color channels
			{
				// some_image.rgba = some_image.bgra
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
		.subresourceRange =                                               // What part of the image to view
			{
				.aspectMask     = aspectFlags,                            // Which aspect of the image to view (e.g. COLOR_BIT for viewing color)
				.baseMipLevel   = 0,                                      // Start mipmap level to view from
				.levelCount     = 1,                                      // Number of mipmap levels to view
				.baseArrayLayer = 0,                                      // Start array level to view from
				.layerCount     = 1                                       // Number of array levels to view
			}
	};

	// Create image view and return it
	VkImageView imageView;
	if (vkCreateImageView(m_mainDevice.logicalDevice, &viewInfo, nullptr, &imageView) != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an Image View!");
	}

	return imageView;
}
