#include "VulkanRenderer.h"

#include <cstring>


VulkanRenderer::VulkanRenderer()
{
}

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
		GetPhysicalDevice();
		CreateLogicalDevice();
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
	// Destroy the logical device
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	mainDevice = {NULL};

	// Destroy the Vulkan Instance
	vkDestroyInstance(m_instance, nullptr);
	m_instance = NULL;
}

void
VulkanRenderer::CreateInstance()
{
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
	std::vector<const char *> instanceExtensions{};

	// Get the required extensions from GLFW
	{
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		// Create a vector to hold the extensions
		for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		{
			instanceExtensions.push_back(glfwExtensions[i]);
		}

		// Check if the instance extensions are supported
		std::string unSupExt{};
		if (!TryInstanceExtensionSupport(&instanceExtensions, unSupExt))
		{
			throw std::runtime_error("VkInstance does not support required extension: " + unSupExt);
		}
	}

	// Creation information for a Vulkan Instance
	VkInstanceCreateInfo createInfo =
	{
		.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,  // The type of this structure
		.pNext                   = nullptr,                                 // Pointer to extension information
		.flags                   = 0,                                       // Reserved for future use
		.pApplicationInfo        = &appInfo,                                // Information about the application
		.enabledLayerCount       = 0,                                       //  | Number of layers to enable (Used for validation)
		.ppEnabledLayerNames     = nullptr,                                 //  | Names of the layers to enable
		.enabledExtensionCount   = (uint32_t) instanceExtensions.size(),    // Number of extensions to enable
		.ppEnabledExtensionNames = instanceExtensions.data()                // Names of the extensions to enable
	};

	// Create the Vulkan Instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
	if (result != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan Instance");
	}
}

void
VulkanRenderer::CreateLogicalDevice()
{
	// Get queue family indices for the physical device
	queueFamilyIndices_t indices = GetQueueFamilies(mainDevice.physicalDevice);

	// Queue the logical device
	float queuePriority = 1.0F;
	VkDeviceQueueCreateInfo queueCreateInfo =
	{
		.sType             = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex  = indices.graphicsFamily,
		.queueCount        = 1,
		.pQueuePriorities  = &queuePriority
	};

	// Logical device creation
	// TIP: Device is Logical device. PhysicalDevice is Physical Device

	VkPhysicalDeviceFeatures physicalDeviceFeatures {};

	VkDeviceCreateInfo deviceCreateInfo =
	{
		.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount     = 1,
		.pQueueCreateInfos        = &queueCreateInfo,
		.enabledExtensionCount    = 0,
		.ppEnabledExtensionNames  = nullptr,
		.pEnabledFeatures         = &physicalDeviceFeatures
	};

	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VkResult::VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a logical device");
	}

	// Queues are created at the same time as the device
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
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

	for (const auto &device: deviceList)
	{
		if (!CheckDeviceSuitable(device)) continue;

		mainDevice.physicalDevice = device;
		break;
	}
}


bool
VulkanRenderer::TryInstanceExtensionSupport(std::vector<const char *> *checkExtensions, std::string &outUnSupExt)
{
	// Number of extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// List of VkExtensionProperties
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// Check if the required extensions are in the available extensions
	for (const auto &checkExtension: *checkExtensions)
	{
		bool hasExtension = false;
		for (const auto &extension: extensions)
		{
			// Determine if the extension is supported
			if (strcmp(checkExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		// Unsupported extension found
		if (!hasExtension)
		{
			outUnSupExt = std::string(checkExtension);
			return false;
		}
	}

	// All required extensions are supported
	return true;
}

bool
VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice device)
{
	// Device's info
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Capabilities
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	queueFamilyIndices_t indices = GetQueueFamilies(device);

	return indices.IsValid();
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
	for (uint32_t i = 0; i < queueFamilyCount; ++i)
	{
		const VkQueueFamilyProperties &queueFamily = queueFamilies[i];

		// Check if the queue family has at least 1 of the required types of queues
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		// Check if the queue family is valid
		if (indices.IsValid()) break;
	}

	return indices;
}
