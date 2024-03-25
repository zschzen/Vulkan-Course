#include "VulkanRenderer.h"

#include <cstring>
#include <array>

#include "VulkanValidation.h"

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
		// Instance Creation
		CreateInstance();
		CreateDebugMessenger();
		CreateSurface();

		// Device Setup
		GetPhysicalDevice();
		CreateLogicalDevice();

		// Swap Chain Creation
		CreateSwapChain();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFramebuffers();

		// Command Pool and Buffer Setup
		CreateCommandPool();
		CreateCommandBuffers();

		// Mesh Model Loading
		{
			std::vector<vertex_t> meshVertices =
			{
				{ { -0.1, -0.4, 0.0 }, { 1.0f, 0.0f, 0.0f } },
				{ { -0.1,  0.4, 0.0 }, { 0.0f, 1.0f, 0.0f } },
				{ { -0.9,  0.4, 0.0 }, { 0.0f, 0.0f, 1.0f } },
				{ { -0.9, -0.4, 0.0 }, { 1.0f, 1.0f, 0.0f } },
			};

			std::vector<vertex_t> meshVertices2 =
			{
				{ { 0.9, -0.4, 0.0 }, { 1.0f, 1.0f, 0.0f } },
				{ { 0.9,  0.4, 0.0 }, { 0.0f, 0.0f, 1.0f } },
				{ { 0.1,  0.4, 0.0 }, { 0.0f, 1.0f, 0.0f } },
				{ { 0.1, -0.4, 0.0 }, { 1.0f, 0.0f, 0.0f } },
			};

			// Index Data
			std::vector<uint32_t> meshIndices =
			{
				0, 1, 2,
				2, 3, 0
			};

			Mesh firstMesh = Mesh(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice,
								  m_graphicsQueue, m_graphicsCommandPool,
								  &meshVertices, &meshIndices);
			Mesh secondMesh = Mesh(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice,
								   m_graphicsQueue, m_graphicsCommandPool,
								   &meshVertices2, &meshIndices);

			// Add to a mesh list
			m_meshList.push_back(firstMesh);
			m_meshList.push_back(secondMesh);

			// Add meshes to the deletion queue
			m_mainDeletionQueue.push_function([&]() -> void
			{
				for (auto &m : m_meshList) m.DestroyVertexBuffer();
				m_meshList.clear();
			});
		}

		// Command Recording
		RecordCommands();

		// Semaphores and Fences
		CreateSemaphores();
	}
	catch (const std::runtime_error &e)
	{
		fprintf(stderr, "[ERROR] %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void
VulkanRenderer::Draw()
{

	// ----------------------------------------- GET NEXT IMAGE -----------------------------------------

	// Wait for given fence to signal (open) from last draw before continuing
	vkWaitForFences(m_mainDevice.logicalDevice, 1, &m_drawFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	// Manually reset (close) fences
	vkResetFences(m_mainDevice.logicalDevice, 1, &m_drawFences[m_currentFrame]);


	// ----------------------------------------- FRAME BUFFER CREATION -----------------------------------------
	uint32_t imageIndex;
	// Get index of next image to be drawn to
	vkAcquireNextImageKHR(m_mainDevice.logicalDevice, m_swapchain, std::numeric_limits<uint64_t>::max(),
						  m_imageAvailableSemaphore[m_currentFrame], VK_NULL_HANDLE, &imageIndex);


	// ----------------------------------------- SUBMIT COMMAND BUFFER TO RENDER --------------------------------

	std::array<VkPipelineStageFlags, 1> waitStages =
	{
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	// Queue submission information
	VkSubmitInfo submitInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,                                          // Number of semaphores to wait on
		.pWaitSemaphores    = &m_imageAvailableSemaphore[m_currentFrame], // Semaphores to wait on
		.pWaitDstStageMask  = waitStages.data(),                       // Semaphores to wait on
		.commandBufferCount = 1,
		.pCommandBuffers    = &m_commandBuffers[imageIndex],          // Command buffer to submit
		.signalSemaphoreCount = 1,                                    // Number of semaphores to signal
		.pSignalSemaphores    = &m_renderFinishedSemaphore[m_currentFrame] // Semaphores to signal
	};

	// Submit command buffer to queue
	VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_drawFences[m_currentFrame]),
			 "Failed to submit command buffer to queue");

	// ----------------------------------------- PRESENT RENDERED IMAGE TO SCREEN --------------------------------
	// Present info
	VkPresentInfoKHR presentInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,                              // Number of semaphores to wait on
		.pWaitSemaphores    = &m_renderFinishedSemaphore[m_currentFrame], // Semaphores to wait on
		.swapchainCount     = 1,                              // Number of swapchains to present to
		.pSwapchains        = &m_swapchain,                    // Swapchains to present images to
		.pImageIndices      = &imageIndex             // Index of images in swapchains to present
	};

	// Present image
	VK_CHECK(vkQueuePresentKHR(m_presentationQueue, &presentInfo), "Failed to present Image!");

	// Increment current frame (limited by MAX_FRAME_DRAWS)
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_DRAWS;
}

void
VulkanRenderer::Cleanup()
{
	if (m_mainDeletionQueue.empty()) return;

	// Wait for a logical device to finish before cleanup
	vkDeviceWaitIdle(m_mainDevice.logicalDevice);

	m_mainDeletionQueue.flush();
}

void
VulkanRenderer::CreateInstance()
{
	// Check if validation layers are available
	if (ENABLE_VALIDATION_LAYERS)
	{
		std::string unSupValLayer{};
		if (!TryCheckValidationLayerSupport(unSupValLayer))
		{
			throw std::runtime_error("Validation layers not supported: " + unSupValLayer);
		}
	}

	// Information about the application.
	// Debugging purposes. Used for developer convenience.
	VkApplicationInfo appInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext              = nullptr,
		.pApplicationName   = "Vulkan App",
		.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),
		.pEngineName        = "No Engine",
		.engineVersion      = VK_MAKE_API_VERSION(1, 0, 0, 0),
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
		createInfo.ppEnabledLayerNames = nullptr;

		createInfo.pNext = nullptr;
	}

	// Create the Vulkan Instance
	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance), "Failed to create Vulkan Instance");

	// Add instance to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	});
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

	VkPhysicalDeviceFeatures physicalDeviceFeatures =
	{
		.samplerAnisotropy = VK_TRUE
	};

	VkDeviceCreateInfo deviceCreateInfo =
	{
		.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount     = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos        = queueCreateInfos.data(),
		.enabledExtensionCount    = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames  = deviceExtensions.data(),
		.pEnabledFeatures         = &physicalDeviceFeatures
	};

	VK_CHECK(vkCreateDevice(m_mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &m_mainDevice.logicalDevice),
			 "Failed to create a logical device");

	// Queues are created at the same time as the device
	vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.presentationFamily, 0, &m_presentationQueue);

	// Add logical device to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkDestroyDevice(m_mainDevice.logicalDevice, nullptr);
		m_mainDevice.logicalDevice = VK_NULL_HANDLE;
	});
}

void
VulkanRenderer::CreateDebugMessenger()
{
	if (!ENABLE_VALIDATION_LAYERS) return;

	// Create the debug messenger
	VkDebugUtilsMessengerCreateInfoEXT createInfo {};
	PopulateDebugMessengerCreateInfo(createInfo);

	// Create the debug messenger
	VK_CHECK(CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger),
			 "Failed to set up debug messenger");

	// Add to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		m_debugMessenger = VK_NULL_HANDLE;
	});
}

void
VulkanRenderer::CreateSurface()
{
	VK_CHECK(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface), "Failed to create Surface!");

	// Add surface to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	});
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
	VK_CHECK(vkCreateSwapchainKHR(m_mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &m_swapchain),
			 "Failed to create a Swapchain!");

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

	// Add swap chain to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		for (const auto &image : m_swapChainImages)
		{
			vkDestroyImageView(m_mainDevice.logicalDevice, image.imageView, nullptr);
		}
		m_swapChainImages.clear();

		vkDestroySwapchainKHR(m_mainDevice.logicalDevice, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	});
}

void
VulkanRenderer::CreateRenderPass()
{
	/// Colour attachment of render pass (e.g. layout(location = 0) in shader)
	VkAttachmentDescription colorAttachment =
	{
		.format         = m_swapChainImageFormat,           // Format to use for the attachment
		.samples        = VK_SAMPLE_COUNT_1_BIT,            // Number of samples to write for multisampling
		.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,      // Describes what to do with the attachment before rendering
		.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,     // Describes what to do with the attachment after rendering
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,  // Describes what to do with the stencil before rendering
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, // Describes what to do with the stencil after rendering
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,        // Image data layout before render pass
		.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR   // Image data layout after render pass
	};

	// Attachment reference uses an attachment index, which refers to index in the attachment list passed to renderPassCreateInfo
	VkAttachmentReference colorAttachmentRef =
	{
		.attachment = 0,                                       // Attachment index that this reference refers to
		.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // Layout to use when the attachment is referred to
	};

	// Information about a particular subpass the render pass is using
	std::array<VkSubpassDescription, 1> subpasses =
	{
		{
			{
				.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS, // Pipeline type subpass is to be bound to
				.colorAttachmentCount    = 1,                               // Number of colour attachments
				.pColorAttachments       = &colorAttachmentRef              // Reference to the colour attachment in the subpass
			}
		}
	};

	/*
	 * Dependency Graph:
	 *
	 * [ VK_SUBPASS_EXTERNAL ] --> Dependency 1 --> [ Subpass 0 ] --> Dependency 2 --> [ VK_SUBPASS_EXTERNAL ]
	 *
	 *
	 * 1. Dependency 1: (Must happen after the image is available)
	 *
	 *    - srcSubpass = VK_SUBPASS_EXTERNAL: This refers to operations that occur outside the subpasses, before Subpass 0.
	 *    - dstSubpass = 0: This is the first subpass in the render pass.
	 *    - srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: The operation occurs at the end of the pipeline.
	 *    - dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT: The operation waits for the color attachment output stage.
	 *    - srcAccessMask = VK_ACCESS_MEMORY_READ_BIT: The memory must be read before the conversion.
	 *    - dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT: The color attachment must be READ or WRITTEN to after the conversion.
	 *
	 * 2. Dependency 2: (Must happen before the image is presented)
	 *
	 *    - srcSubpass = 0: This is the first subpass in the render pass.
	 *    - dstSubpass = VK_SUBPASS_EXTERNAL: This refers to operations that occur outside the subpasses, after Subpass 0.
	 *    - srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT: The operation occurs at the color attachment output stage.
	 *    - dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: The operation waits for the end of the pipeline.
	 *    - srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT: The color attachment must be READ or WRITTEN to before the conversion.
	 *    - dstAccessMask = VK_ACCESS_MEMORY_READ_BIT: The memory must be read after the conversion.
	 *
	 *
	 * These dependencies ensure that the render pass doesn't begin until the image is available and that rendering is complete before transitioning to the next stage.
	 * They dictate the order of operations and ensure that all necessary data is correctly processed and available when needed.
	 * They allow for image layout transitions, define execution dependencies between subpasses, and manage memory dependencies.
	 * This ensures correct rendering results, avoids crashes, and allows for efficient use of resources.
	 */
	std::array<VkSubpassDependency, 2> subpassDependencies =
	{
		{
			{
				.srcSubpass      = VK_SUBPASS_EXTERNAL,                                                        // Anything that takes place outside the subpasses
				.dstSubpass      = 0,                                                                          // ID of the subpass that the dependency is transitioning to
				.srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,                                       // Pipeline stage that the operation occurs at
				.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,                              // Pipeline stage that the operation waits on
				.srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT,                                                  // Memory operation. Means: Must be read from before the conversion
				.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // Memory operation. Means: must be before the attempt to read and write to the attachment
				.dependencyFlags = 0                                                                           // Could this happen in certain regions of the pipeline?
			},
			{
				.srcSubpass      = 0,
				.dstSubpass      = VK_SUBPASS_EXTERNAL,
				.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				.srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT,
				.dependencyFlags = 0
			}
		}
	};

	// Create info for Render Pass
	VkRenderPassCreateInfo renderPassCreateInfo =
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments    = &colorAttachment,
		.subpassCount    = static_cast<uint32_t>(subpasses.size()),
		.pSubpasses      = subpasses.data(),
		.dependencyCount = static_cast<uint32_t>(subpassDependencies.size()),
		.pDependencies   = subpassDependencies.data()
	};

	// Create the render pass
	VK_CHECK(vkCreateRenderPass(m_mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &m_renderPass),
			 "Failed to create a Render Pass!");

	// Add to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkDestroyRenderPass(m_mainDevice.logicalDevice, m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
	});
}

void
VulkanRenderer::CreateGraphicsPipeline()
{
	// Read in SPIR-V bytecode
	auto vertShaderCode = ReadFile("Assets/Shader/vert.spv");
	auto fragShaderCode = ReadFile("Assets/Shader/frag.spv");

	// Shader Module is a wrapper object for the shader bytecode
	VkShaderModule vertexShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule   = CreateShaderModule(fragShaderCode);


	/* ----------------------------------------- Shader Stage Creation Information ----------------------------------------- */

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
	{
		{
			// Vertex stage creation information
			{
				.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage               = VK_SHADER_STAGE_VERTEX_BIT,  // Shader stage name
				.module              = vertexShaderModule,          // Shader module to be used by stage
				.pName               = "main"                       // Entry point in the shader
			},
			// Fragment stage creation information
			{
				.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module              = fragShaderModule,
				.pName               = "main"
			}
		}
	};


	/* ----------------------------------------- Vertex Input ----------------------------------------- */

	VkVertexInputBindingDescription bindingDescription    = vertex_t::GetBindingDescription();
	vertex_t::attributeDescriptions attributeDescriptions = vertex_t::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo =
	{
		.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount   = 1,                                                   // Number of vertex binding descriptions
		.pVertexBindingDescriptions      = &bindingDescription,                                 // List of vertex binding descriptions (data spacing/stride information)
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()), // Number of vertex attribute descriptions
		.pVertexAttributeDescriptions    = attributeDescriptions.data()                         // List of vertex attribute descriptions (data format and where to bind to from)
	};


	/* ----------------------------------------- Input Assembly ----------------------------------------- */

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // Primitive type to assemble vertices as
		.primitiveRestartEnable = VK_FALSE                             // Allow overriding of "strip" topology to start new primitives
	};

	/*
	 * VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST treats each set of three vertices as an independent triangle.
	 * For example, if we have six vertices (v0, v1, v2, v3, v4, v5), it will form two triangles: (v0, v1, v2) and (v3, v4, v5).
	 *
	 * v0     v3
	 *  *-----*
	 *  |   / |
	 *  |  /  |
	 *  | /   |
	 *  |/    |
	 *  *-----*
	 * v2     v5
	 */


	/* ----------------------------------------- Viewport & Scissor ----------------------------------------- */

	// Viewport is the area that the output will be rendered to
	VkViewport viewport =
	{
		.x        = 0.0F,                             // x start coordinate
		.y        = 0.0F,                             // y start coordinate

		.width    = (float)m_swapChainExtent.width,   // width of the framebuffer
		.height   = (float)m_swapChainExtent.height,  // height of the framebuffer

		.minDepth = 0.0F,                             // Min depth of the framebuffer
		.maxDepth = 1.0F                              // Max depth of the framebuffer
	};

	// Crop the framebuffer
	VkRect2D scissor =
	{
		.offset = { 0, 0 },          // Offset to use region from
		.extent = m_swapChainExtent  // Extent to describe region to use, starting at offset
	};

	// Combine viewport and scissor into a viewport state
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
	{
		.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,          // Number of viewports to use
		.pViewports    = &viewport,  // List of viewports to use
		.scissorCount  = 1,          // Number of scissor rectangles to use
		.pScissors     = &scissor    // List of scissor rectangles to use
	};


	/* ----------------------------------------- Dynamic States ----------------------------------------- */

	// Dynamic states to enable
	// !WARNING! If you are resizing the window, you need to recreate the swap chain,
	// 			 swap chain images, and any image views associated with output attachments to the swap chain
	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,  // Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		VK_DYNAMIC_STATE_SCISSOR,   // Dynamic Scissor  : Can resize in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		VK_DYNAMIC_STATE_LINE_WIDTH   // Dynamic Line Width : Can resize in command buffer with vkCmdSetLineWidth(commandBuffer, 1.0F);
	};

	// Dynamic state creation information
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo =
	{
		.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), // Number of dynamic states to enable
		.pDynamicStates    = dynamicStates.data()                         // List of dynamic states to enable
	};


	/* ----------------------------------------- Depth Stencil ----------------------------------------- */

	// Depth and stencil testing
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo =
	{
		.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable       = VK_TRUE,                  // Enable checking depth to determine fragment write
		.depthWriteEnable      = VK_TRUE,                  // Enable writing to the depth buffer
		.depthCompareOp        = VK_COMPARE_OP_LESS,       // Comparison operation that allows an overwrite (is in front)
		.depthBoundsTestEnable = VK_FALSE,                 // Depth bounds test : Does the depth value exist between two bounds
		.stencilTestEnable     = VK_FALSE,                 // Enable checking stencil value
		.front                 = {},                       // Stencil operations for front-facing triangles
		.back                  = {},                       // Stencil operations for back-facing triangles
		.minDepthBounds        = 0.0F,                     // Min depth bounds
		.maxDepthBounds        = 1.0F,                     // Max depth bounds
	};


	/* ----------------------------------------- Rasterizer ----------------------------------------- */

	// How to draw the polygons
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo =
	{
		.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable        = VK_FALSE,                 // Change if fragments beyond near/far planes are clamped (default) or discarded
		.rasterizerDiscardEnable = VK_FALSE,                 // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
		.polygonMode             = VK_POLYGON_MODE_FILL,     // How to handle filling points between vertices
		.cullMode                = VK_CULL_MODE_BACK_BIT,    // Which face of a triangle to cull
		.frontFace               = VK_FRONT_FACE_CLOCKWISE,  // Winding to determine which side is front
		.depthBiasEnable         = VK_FALSE,                 // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
		.lineWidth               = 1.0F,                     // How thick lines should be when drawn
	};



	/* ----------------------------------------- Multisampling ----------------------------------------- */

	// How to handle multisampling
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo =
	{
		.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,  // Number of samples to use per fragment
		.sampleShadingEnable   = VK_FALSE,               // Enable multisample shading or not
	};


	/* ----------------------------------------- Colour Blending ----------------------------------------- */

	// How to handle the colours
	VkPipelineColorBlendAttachmentState colorBlendAttachment =
	{
		.blendEnable         = VK_TRUE,                                             // Enable blending
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,                           // How to handle blending of new colour
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,                 // How to handle blending of old colour
		.colorBlendOp        = VK_BLEND_OP_ADD,                                     // Type of blend operation to use
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,                                 // How to handle blending of new alpha
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,                                // How to handle blending of old alpha
		.alphaBlendOp        = VK_BLEND_OP_ADD,                                     // Type of blend operation to use for alpha
		.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT  // Which colours to apply the blend to
		                     | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	/*
	 * Blend Equation:
	 *
	 * newColor.rgb = (srcColourBlendFactor * newColor)    colourBlendOp    (dstColourBlendFactor * oldColor)
	 * newColor.a   = (srcAlphaBlendFactor * newAlpha)     alphaBlendOp     (dstAlphaBlendFactor * oldAlpha)
	 *
	 * Summarised:
	 *
	 * (VK_BLEND_FACTOR_SRC_ALPHA * newColor) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * oldColor)
	 * ( newAlpha + newColor ) + ( ( 1 - newAlpha ) * oldColor )
	 */

	// How to handle all the colours and alpha
	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo =
	{
		.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable     = VK_FALSE,                   // Alternative to calculations is to use logical operations
		.logicOp           = VK_LOGIC_OP_COPY,           // What logical operation to use
		.attachmentCount   = 1,                          // Number of colour blend attachments
		.pAttachments      = &colorBlendAttachment,      // Information about how to handle blending
		.blendConstants    = { 0.0F, 0.0F, 0.0F, 0.0F }  // (Optional) Constants to use for blending [VK_BLEND_FACTOR_CONSTANT_COLOR]
	};


	/* ----------------------------------------- Pipeline Layout ----------------------------------------- */

	/// A pipeline layout object describes the complete set of resources that can be accessed by a pipeline.

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount         = 0,        // Number of layouts to use with pipeline
		.pSetLayouts            = nullptr,  // Layouts to set to use with pipeline
		.pushConstantRangeCount = 0,        // Number of push constant ranges
		.pPushConstantRanges    = nullptr   // Push constant ranges to use with pipeline
	};

	VK_CHECK(vkCreatePipelineLayout(m_mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout),
			 "Failed to create pipeline layout");


	/* ----------------------------------------- Create Pipeline ----------------------------------------- */

	VkGraphicsPipelineCreateInfo pipelineCreateInfo =
	{
		.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount          = static_cast<uint32_t>(shaderStages.size()),
		.pStages             = shaderStages.data(),

		// Actual states creations
		.pVertexInputState   = &vertexInputCreateInfo,
		.pInputAssemblyState = &inputAssemblyCreateInfo,
		.pViewportState      = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizerCreateInfo,
		.pMultisampleState   = &multisamplingCreateInfo,
		.pDepthStencilState  = &depthStencilStateCreateInfo,
		.pColorBlendState    = &colorBlendCreateInfo,
		.pDynamicState       = &dynamicStateCreateInfo,

		.layout              = m_pipelineLayout,                                  // Pipeline layout to use with render pass
		.renderPass          = m_renderPass,                                      // Render pass description the pipeline is compatible with
		.subpass             = 0,                                                 // Index of the subpass to use with this pipeline

		// Pipeline derivatives : Can create multiple pipelines that derive from one another for optimisation
		.basePipelineHandle  = VK_NULL_HANDLE,                                    // Pipeline to derive from
		.basePipelineIndex   = -1                                                 // Index of the base pipeline to derive from
	};

	VK_CHECK(vkCreateGraphicsPipelines(m_mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
									   &m_graphicsPipeline),
			 "Failed to create Graphics Pipeline");

	// Destroy shader modules
	vkDestroyShaderModule(m_mainDevice.logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(m_mainDevice.logicalDevice, vertexShaderModule, nullptr);

	// Add pipeline to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkDestroyPipeline(m_mainDevice.logicalDevice, m_graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_mainDevice.logicalDevice, m_pipelineLayout, nullptr);
		m_graphicsPipeline = VK_NULL_HANDLE;
		m_pipelineLayout   = VK_NULL_HANDLE;
	});
}

void
VulkanRenderer::CreateFramebuffers()
{
	// Resize the framebuffer count to the swap chain image count
	m_swapChainFramebuffers.resize(m_swapChainImages.size());

	// Create a framebuffer for each swap chain image
	for (size_t i = 0; i < m_swapChainFramebuffers.size(); ++i)
	{
		// TIP: The attachments here are the same ones were just looking in the render pass.
		//		See CreateRenderPass() for those attachments.
		std::array<VkImageView, 1> attachments =
		{
			m_swapChainImages[i].imageView
		};

		VkFramebufferCreateInfo framebufferCreateInfo =
		{
			.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass      = m_renderPass,                               // Render Pass layout the framebuffer will be used with
			.attachmentCount = static_cast<uint32_t>(attachments.size()),  // Number of attachments
			.pAttachments    = attachments.data(),                         // Attachments to be used by the framebuffer (Actually the things that are being attached, not the ones from render pass)
			.width           = m_swapChainExtent.width,                    // Width of the framebuffer
			.height          = m_swapChainExtent.height,                   // Height of the framebuffer
			.layers          = 1                                           // Number of layers to be used by the framebuffer
		};

		VK_CHECK(vkCreateFramebuffer(m_mainDevice.logicalDevice, &framebufferCreateInfo, nullptr,
									 &m_swapChainFramebuffers[i]),
				 "Failed to create a Framebuffer!");
	}

	// Add framebuffers to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		for (const auto &framebuffer : m_swapChainFramebuffers)
		{
			vkDestroyFramebuffer(m_mainDevice.logicalDevice, framebuffer, nullptr);
		}
		m_swapChainFramebuffers.clear();
	});
}

void
VulkanRenderer::CreateCommandPool()
{
	// Get the queue family indices for the physical device
	const queueFamilyIndices_t queueFamilyIndices = GetQueueFamilies(m_mainDevice.physicalDevice);

	VkCommandPoolCreateInfo poolCreateInfo =
	{
		.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,           // Allow the buffer to be rerecorded
		.queueFamilyIndex = static_cast<uint32_t>(queueFamilyIndices.graphicsFamily),  // Queue Family type that buffers from this command pool will use
	};

	// Create a Graphics Queue Family Command Pool
	VK_CHECK(vkCreateCommandPool(m_mainDevice.logicalDevice, &poolCreateInfo,
								 nullptr, &m_graphicsCommandPool), "Failed to create a command pool");

	// Add command pool to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkDestroyCommandPool(m_mainDevice.logicalDevice, m_graphicsCommandPool, nullptr);
		m_graphicsCommandPool = VK_NULL_HANDLE;
	});
}

void
VulkanRenderer::CreateCommandBuffers()
{
	// Resize command buffer count to have one for each framebuffer
	m_commandBuffers.resize(m_swapChainFramebuffers.size());

	AllocateCommandBuffer(m_mainDevice.logicalDevice, m_graphicsCommandPool,
						  *m_commandBuffers.data(),
						  VK_COMMAND_BUFFER_LEVEL_PRIMARY,
						  static_cast<uint32_t>( m_commandBuffers.size() )
	);

	// Add command buffers to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkFreeCommandBuffers(m_mainDevice.logicalDevice, m_graphicsCommandPool,
							 static_cast<uint32_t>(m_commandBuffers.size()),
							 m_commandBuffers.data()
		);

		m_commandBuffers.clear();
	});
}

void
VulkanRenderer::CreateSemaphores()
{
	// Resize the semaphores to fit all the swap chain images
	m_imageAvailableSemaphore.resize(MAX_FRAME_DRAWS);
	m_renderFinishedSemaphore.resize(MAX_FRAME_DRAWS);
	m_drawFences.resize(MAX_FRAME_DRAWS);

	/// Semaphore creation information
	VkSemaphoreCreateInfo semaphoreCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	/// Fence creation information
	VkFenceCreateInfo fenceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT         // Fence starts "signaled" (green flag) so we don't have to wait on the first frame
	};

	for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
	{
		// Create Semaphores
		VK_CHECK(vkCreateSemaphore(m_mainDevice.logicalDevice, &semaphoreCreateInfo,
								   nullptr, &m_imageAvailableSemaphore[i]), "Failed to create a Semaphore!");
		VK_CHECK(vkCreateSemaphore(m_mainDevice.logicalDevice, &semaphoreCreateInfo,
								   nullptr, &m_renderFinishedSemaphore[i]), "Failed to create a Semaphore!");

		// Create Fences
		VK_CHECK(vkCreateFence(m_mainDevice.logicalDevice, &fenceCreateInfo,
							   nullptr, &m_drawFences[i]), "Failed to create a Fence!");
	}

	// Add semaphores to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
		{
			vkDestroySemaphore(m_mainDevice.logicalDevice, m_imageAvailableSemaphore[i], nullptr);
			vkDestroySemaphore(m_mainDevice.logicalDevice, m_renderFinishedSemaphore[i], nullptr);
			vkDestroyFence(m_mainDevice.logicalDevice, m_drawFences[i], nullptr);
		}
		m_imageAvailableSemaphore.clear();
		m_renderFinishedSemaphore.clear();
	});
}


void
VulkanRenderer::RecordCommands()
{
	// Command buffer details
	VkCommandBufferBeginInfo bufferBeginInfo =
	{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,                                       // Pointer to extension-related structures
			.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,  // Buffer can be resubmitted when it has already been submitted and is awaiting execution
			.pInheritanceInfo = nullptr                             // Secondary command buffer details
	};

	std::array<VkClearValue, 1> clearValues =
	{
		{
			{
				.color = {0.16F, 0.16F, 0.21F, 1.0F}     // Dracula Background Colour
			}
		}
	};

	// Info about how to begin each render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo =
	{
		.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass        = m_renderPass,                               // Render Pass to begin recording commands for
		.renderArea        =                                             // Size of region to render to (starting point)
		{
			.offset = { 0, 0 },                                          // Offset to start rendering at (in pixels)
			.extent = m_swapChainExtent                                  // Extent to render
		},
		.clearValueCount   = static_cast<uint32_t>(clearValues.size()),  // Number of clear values to clear the attachments
		.pClearValues      = clearValues.data()                          // Clear values to use for clear the attachments
	};

	for (size_t i = 0; i < m_commandBuffers.size(); ++i)
	{
		// Framebuffer to use in the render pass
		renderPassBeginInfo.framebuffer = m_swapChainFramebuffers[i];


		// Start recording commands to the command buffer
		VK_CHECK(vkBeginCommandBuffer(m_commandBuffers[i], &bufferBeginInfo), "Failed to start recording a command buffer");


			/* ------------------------------------- Begin the render pass -------------------------------------- */
			// Begin the render pass
			//	- VK_SUBPASS_CONTENTS_INLINE: All the render commands will be embedded in the primary command buffer. Will be inlined.
			//	- VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render commands will be executed from secondary command buffers.
			vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


				// ------- Viewport -------
				VkViewport viewport =
				{
					.x = 0.0f,
					.y = 0.0f,
					.width = (float)m_swapChainExtent.width,
					.height = (float)m_swapChainExtent.height,
					.minDepth = 0.0f,
					.maxDepth = 1.0f
				};
				vkCmdSetViewport(m_commandBuffers[i], 0, 1, &viewport);

				// ------- Scissor -------
				VkRect2D scissor = {
						.offset = {0, 0},
						.extent = m_swapChainExtent
				};
				vkCmdSetScissor(m_commandBuffers[i], 0, 1, &scissor);

				// ------- Bind Pipeline -------
				vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

				// ------- Draw -------
				for (size_t j = 0; j < m_meshList.size(); ++j)
				{
					VkBuffer vertexBuffers[] = { m_meshList[j].GetVertexBuffer() };                  // Buffers to bind
					VkDeviceSize offsets[] = { 0 };                                                  // Offsets into buffers being bound
					vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);       // Command to bind vertex buffer before drawing with them

					// Bind the mesh index buffer with 0 offset and using the mesh's index count
					vkCmdBindIndexBuffer(m_commandBuffers[i], m_meshList[j].GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

					// Execute the pipeline
					vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(m_meshList[j].GetIndexCount()), 1, 0, 0, 0);
				}


			/* ------------------------------------- End the render pass -------------------------------------- */
			vkCmdEndRenderPass(m_commandBuffers[i]);


		// Stop recording commands to the command buffer
		VK_CHECK(vkEndCommandBuffer(m_commandBuffers[i]), "Failed to stop recording a command buffer");
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
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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
	{
		std::string unSupDevExt{};
		if (!TryCheckDeviceExtensionSupport(device, unSupDevExt))
		{
			throw std::runtime_error("Device does not support required extension: " + unSupDevExt);
		}
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

// TODO: Cache the queue families
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
	swapChainDetails_t swapChainDetails {0};

	/* ----------------------------------------- Capabilities ----------------------------------------------- */

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &swapChainDetails.surfaceCapabilities);

	/* ----------------------------------------- Surface Formats -------------------------------------------- */

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	// If formats returned, get the list of formats
	if (formatCount != 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, swapChainDetails.formats.data());
	}

	/* ----------------------------------------- Presentation Modes ----------------------------------------- */

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
VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const
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
	VK_CHECK(vkCreateImageView(m_mainDevice.logicalDevice, &viewInfo, nullptr, &imageView),
			 "Failed to create an Image View!");

	return imageView;
}

VkShaderModule
VulkanRenderer::CreateShaderModule(const std::vector<char> &code) const
{
	// Shader module creation info
	VkShaderModuleCreateInfo createInfo =
	{
		.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = code.size(),
		.pCode    = reinterpret_cast<const uint32_t *> ( code.data() )
	};

	// Create shader module
	VkShaderModule shaderModule;
	VK_CHECK(vkCreateShaderModule(m_mainDevice.logicalDevice, &createInfo, nullptr, &shaderModule),
			 "Failed to create a shader module!");

	return shaderModule;
}
