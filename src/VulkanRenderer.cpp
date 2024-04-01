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
		CreateDepthBufferImage();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreatePushConstantRange();
		CreateGraphicsPipeline();
		CreateFramebuffers();

		// Command Pool and Buffer Setup
		CreateCommandPool();

		// Mesh Model Loading
		{
			std::vector<vertex_t> meshVertices =
			{
				{ { -0.4,  0.4, 0.0 }, { 1.0f, 0.33f, 0.33f } },
				{ { -0.4, -0.4, 0.0 }, { 1.0f, 0.33f, 0.33f } },
				{ {  0.4, -0.4, 0.0 }, { 1.0f, 0.33f, 0.33f } },
				{ {  0.4,  0.4, 0.0 }, { 1.0f, 0.33f, 0.33f } },
			};

			std::vector<vertex_t> meshVertices2 =
			{
				{ { -0.25,  0.6, 0.0 }, { 0.55f, 0.91f, 0.99f } },
				{ { -0.25, -0.6, 0.0 }, { 0.55f, 0.91f, 0.99f } },
				{ {  0.25, -0.6, 0.0 }, { 0.55f, 0.91f, 0.99f } },
				{ {  0.25,  0.6, 0.0 }, { 0.55f, 0.91f, 0.99f } },
			};

			// Index Data
			std::vector<uint32_t> meshIndices =
			{
				0, 1, 2,
				2, 3, 0
			};

			Mesh firstMesh = Mesh(m_mainDevice, m_graphicsQueue, m_graphicsCommandPool, &meshVertices, &meshIndices);
			Mesh secondMesh = Mesh(m_mainDevice, m_graphicsQueue, m_graphicsCommandPool, &meshVertices2, &meshIndices);

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

		CreateCommandBuffers();
		// Create Uniform Buffers
		//AllocateDynamicBufferTransferSpace(); // Create transfer space for dynamic uniform data. Unneeded for Push Constants
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();

		// Semaphores and Fences
		CreateSemaphores();

		// Set up the UBOs
		CreateViewProjUBO();
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
	/* ----------------------------------------- GET NEXT IMAGE ----------------------------------------- */

	// Wait for given fence to signal (open) from the last draw before continuing
	VK_CHECK(vkWaitForFences(m_mainDevice.logicalDevice, 1, &m_drawFences[m_currentFrame], VK_TRUE, UINT64_MAX),
			 "Failed to wait for a fence to signal that it is available for re-use");
	// Manually reset (close) fences
	VK_CHECK(vkResetFences(m_mainDevice.logicalDevice, 1, &m_drawFences[m_currentFrame]),
			 "Failed to reset fences!");


	/* ----------------------------------------- FRAME BUFFER CREATION ----------------------------------------- */
	uint32_t imageIndex;
	// Get index of next image to be drawn to
	vkAcquireNextImageKHR(m_mainDevice.logicalDevice, m_swapchain, UINT64_MAX,
						  m_imageAvailableSemaphore[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

	/* ----------------------------------------- UPDATE UNIFORM BUFFER ----------------------------------------- */

	RecordCommands(m_commandBuffers[imageIndex], imageIndex);
	UpdateUniformBuffers(imageIndex);

	/* ----------------------------------------- SUBMIT COMMAND BUFFER TO RENDER -------------------------------- */

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	// Queue submission information
	VkSubmitInfo submitInfo =
	{
		.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,

		.waitSemaphoreCount   = 1,                                          // Number of semaphores to wait on
		.pWaitSemaphores      = &m_imageAvailableSemaphore[m_currentFrame], // Semaphores to wait on
		.pWaitDstStageMask    = waitStages,                                 // Semaphores to wait on

		.commandBufferCount   = 1,
		.pCommandBuffers      = &m_commandBuffers[imageIndex],              // Command buffer to submit

		.signalSemaphoreCount = 1,                                          // Number of semaphores to signal
		.pSignalSemaphores    = &m_renderFinishedSemaphore[m_currentFrame]  // Semaphores to signal
	};

	// Submit command buffer to queue
	VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_drawFences[m_currentFrame]),
			 "Failed to submit command buffer to queue");

	/* ----------------------------------------- PRESENT RENDERED IMAGE TO SCREEN -------------------------------- */

	// Present info
	VkPresentInfoKHR presentInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

		.waitSemaphoreCount = 1,                                          // Number of semaphores to wait on
		.pWaitSemaphores    = &m_renderFinishedSemaphore[m_currentFrame], // Semaphores to wait on

		.swapchainCount     = 1,                                          // Number of swapchains to present to
		.pSwapchains        = &m_swapchain,                               // Swapchains to present images to

		.pImageIndices      = &imageIndex                                 // Index of images in swapchains to present
	};

	// Present image
	VkResult presentResult = vkQueuePresentKHR(m_presentationQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_framebufferResized)
	{
		m_framebufferResized = false;
		RecreateSwapChain();
	}

	// Increment current frame (limited by MAX_FRAME_DRAWS)
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_DRAWS;
}

// TODO: Get rid off deletion queues and use arrays of vulkan handles
void
VulkanRenderer::Cleanup()
{
	if (m_mainDeletionQueue.empty()) return;

	// Wait for a logical device to finish before cleanup
	vkDeviceWaitIdle(m_mainDevice.logicalDevice);

	CleanupFramebuffers();
	CleanupDepthBuffer();

	// Clean up swap chain
	CleanupSwapChain();

	// Clean up the leftovers
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
		createInfo.enabledLayerCount   = 0;
		createInfo.ppEnabledLayerNames = nullptr;

		createInfo.pNext               = nullptr;
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
	VK_CHECK(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface),
			 "Failed to create Surface!");

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
}

void
VulkanRenderer::CreateViewProjUBO()
{
	m_ubo_vp.proj = glm::perspective(glm::radians(45.0f), (float)m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f, 100.0f);
	m_ubo_vp.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Flip the Y coordinate. Vulkan has inverted Y coordinates compared to OpenGL
	m_ubo_vp.proj[1][1] *= -1;
}

void
VulkanRenderer::RecreateSwapChain()
{
	// Get the size of the window
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	// Wait for logical device to finish before continuing
	vkDeviceWaitIdle(m_mainDevice.logicalDevice);

	// Clear
	CleanupSwapChain();
	CleanupFramebuffers();
	CleanupDepthBuffer();

	// Recreate the swap chain
	CreateSwapChain();
	CreateDepthBufferImage();
	CreateFramebuffers();

	CreateViewProjUBO();
}

void
VulkanRenderer::CreateRenderPass()
{
	/* ------------------------------------------- ATTACHMENTS ------------------------------------------- */

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

	// Depth attachment of render pass
	VkAttachmentDescription depthAttachment =
	{
		.format         = m_depthFormat,
		.samples        = VK_SAMPLE_COUNT_1_BIT,
		.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	/* ------------------------------------------- REFERENCES ------------------------------------------- */

	// Attachment reference uses an attachment index, which refers to index in the attachment list passed to renderPassCreateInfo
	VkAttachmentReference colorAttachmentRef =
	{
		.attachment = 0,                                       // Attachment index that this reference refers to
		.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // Layout to use when the attachment is referred to
	};

	// Depth attachment reference
	VkAttachmentReference depthAttachmentRef =
	{
		.attachment = 1,                                               // Attachment index that this reference refers to
		.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // Layout to use when the attachment is referred to
	};

	/* ------------------------------------------- SUBPASS ------------------------------------------- */

	// Information about a particular subpass the render pass is using
	std::array<VkSubpassDescription, 1> subpasses =
	{
		{
			{
				.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS, // Pipeline type subpass is to be bound to
				.colorAttachmentCount    = 1,                               // Number of colour attachments
				.pColorAttachments       = &colorAttachmentRef,             // Reference to the colour attachment in the subpass
				.pDepthStencilAttachment = &depthAttachmentRef              // Reference to the depth attachment in the subpass
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

	/* ------------------------------------------- RENDER PASS CREATION ------------------------------------------- */

	// The elements ordered in the array are the attachments, so it must match the attachment in the VkAttachmentReference
	std::vector<VkAttachmentDescription> renderPassAttachments = { colorAttachment, depthAttachment };

	// Create info for Render Pass
	VkRenderPassCreateInfo renderPassCreateInfo =
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,

		.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size()),
		.pAttachments    = renderPassAttachments.data(),

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
VulkanRenderer::CreateDescriptorSetLayout()
{
	/* ----------------------------------------- UNIFORM VALUES DESCRIPTOR SET LAYOUT ----------------------------------------- */

	// View Projection Descriptor
	VkDescriptorSetLayoutBinding vpLayoutBinding =
	{
		.binding            = 0,                                 // Binding point in shader
		.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // Type of descriptor (uniform, dynamic uniform, image sampler, ...)
		.descriptorCount    = 1,                                 // Number of descriptors for binding
		.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,        // Shader stage to bind to
		.pImmutableSamplers = VK_NULL_HANDLE                     // For texture: Can make sampler data unchangeable
	};

	/*
	 * LEGACY CODE: Dynamic Uniform Buffer
	 *
	// Model Descriptor
	VkDescriptorSetLayoutBinding modelLayoutBinding =
	{
		.binding            = 1,
		.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.descriptorCount    = 1,
		.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = VK_NULL_HANDLE
	};
	*/

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

	/* ----------------------------------------- CREATE DESCRIPTOR SET LAYOUT ----------------------------------------- */

	// Create a descriptor set layout with given bindings
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo =
	{
		.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(layoutBindings.size()),
		.pBindings    = layoutBindings.data()
	};

	// Create the descriptor set layout
	VK_CHECK(vkCreateDescriptorSetLayout(m_mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &m_descriptorSetLayout),
			 "Failed to create a Descriptor Set Layout!");

	// Add to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkDestroyDescriptorSetLayout(m_mainDevice.logicalDevice, m_descriptorSetLayout, nullptr);
		m_descriptorSetLayout = VK_NULL_HANDLE;
	});
}

void
VulkanRenderer::CreatePushConstantRange()
{
	// Define push constant values (no 'create' needed)
	m_pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Shader stage push constant will go to
	m_pushConstantRange.offset     = 0;                          // Offset into given data to pass to push constant
	m_pushConstantRange.size       = sizeof(model_t);            // Size of data being passed
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
		.depthWriteEnable      = VK_TRUE,                  // Enable writing to the depth buffer (to replace old values)
		.depthCompareOp        = VK_COMPARE_OP_LESS,       // Comparison operation that allows an overwriting (is in front)
		.depthBoundsTestEnable = VK_FALSE,                 // Depth bounds test: Does the depth value exist between two bounds
		.stencilTestEnable     = VK_FALSE,                 // Enable checking stencil value
		.front                 = {},                       // Stencil operations for front-facing triangles
		.back                  = {},                       // Stencil operations for back-facing triangles
		.minDepthBounds        = 0.0F,                     // Min depth bounds
		.maxDepthBounds        = 1.0F,                     // Max depth bounds
	};

	/**
	 * Depth Testing (depthCompareOp):
	 *
	 * - VK_COMPARE_OP_NEVER            : Always pass the depth test
	 * - VK_COMPARE_OP_LESS             : Pass if the new depth is less than the old depth
	 * - VK_COMPARE_OP_EQUAL            : Pass if the new depth is equal to the old depth
	 * - VK_COMPARE_OP_LESS_OR_EQUAL    : Pass if the new depth is less than or equal to the old depth
	 * - VK_COMPARE_OP_GREATER          : Pass if the new depth is greater than the old depth
	 * - VK_COMPARE_OP_NOT_EQUAL        : Pass if the new depth is not equal to the old depth
	 * - VK_COMPARE_OP_GREATER_OR_EQUAL : Pass if the new depth is greater than or equal to the old depth
	 * - VK_COMPARE_OP_ALWAYS           : Always pass the depth test
	 */


	/* ----------------------------------------- Rasterizer ----------------------------------------- */

	// How to draw the polygons
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo =
	{
		.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable        = VK_FALSE,                         // Change if fragments beyond near/far planes are clamped (default) or discarded
		.rasterizerDiscardEnable = VK_FALSE,                         // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
		.polygonMode             = VK_POLYGON_MODE_FILL,             // How to handle filling points between vertices
		.cullMode                = VK_CULL_MODE_BACK_BIT,            // Which face of a triangle to cull
		.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,  // Winding to determine which side is front
		.depthBiasEnable         = VK_FALSE,                         // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)
		.lineWidth               = 1.0F,                             // How thick lines should be when drawn
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

		.setLayoutCount         = 1,                       // Number of descriptor set layouts
		.pSetLayouts            = &m_descriptorSetLayout,  // Pointer to array of descriptor set layouts

		.pushConstantRangeCount = 1,                       // Number of push constant ranges
		.pPushConstantRanges    = &m_pushConstantRange     // Pointer to array of push constant ranges
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
VulkanRenderer::CreateDepthBufferImage()
{
	// Get supported format for depth buffer
	m_depthFormat = ChooseSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },  // Formats to check
		VK_IMAGE_TILING_OPTIMAL,                                                              // Tiling mode of image
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT                                        // Format feature
	);

	// Create depth buffer image
	m_depthBufferImage = CreateImage(m_swapChainExtent.width, m_swapChainExtent.height, m_depthFormat,
									 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
									 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,  // We don't have to modify this memory from the CPU
									 &m_depthBufferImageMemory);           // Pointer to the image memory

	// Create depth buffer image view
	m_depthBufferImageView = CreateImageView(m_depthBufferImage, m_depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
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
		std::array<VkImageView, 2> attachments =
		{
			// WARNING: The order of the attachments should match the order of the render pass attachments!
			m_swapChainImages[i].imageView,  // Color Attachment is the view of the image
			m_depthBufferImageView           // Depth Attachment is the image view of the depth buffer
		};

		/**
		 * Why multiple swap chain images and only one depth buffer?
		 * - When SwapChains are finished, they are presented to the screen.
		 *    - On Finish, depth buffer is available for the next frame, immediately.
		 */

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
}

void
VulkanRenderer::CreateCommandPool()
{
	// Get the queue family indices for the physical device
	const queueFamilyIndices_t queueFamilyIndices = GetQueueFamilies(m_mainDevice.physicalDevice);

	VkCommandPoolCreateInfo poolCreateInfo =
	{
		.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,           // Allow the command buffer can be reset, so they can be re-recorded
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

	VkCommandBufferAllocateInfo commandBufferAllocInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = m_graphicsCommandPool,  // Command pool to allocate from
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,  // Primary (buffer can be submitted to queue for execution), Secondary (buffer cannot be submitted directly, but can be called from primary buffer)
		.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size())
	};

	// Allocate command buffers and place handles in array of buffers
	VK_CHECK(vkAllocateCommandBuffers(m_mainDevice.logicalDevice, &commandBufferAllocInfo, m_commandBuffers.data()),
			 "Failed to allocate Command Buffers!");

	// Add command buffers to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkFreeCommandBuffers(m_mainDevice.logicalDevice, m_graphicsCommandPool,
							 static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
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
	VkSemaphoreCreateInfo semaphoreCreateInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

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
		m_drawFences.clear();
	});
}

void
VulkanRenderer::CreateUniformBuffers()
{
	// View Projection buffer size
	VkDeviceSize vpBufferSize = sizeof(ubo_view_proj_t);

	// Model buffer size
	//VkDeviceSize modelBufferSize = m_modelUniformAlignment * MAX_OBJECTS;

	// One uniform buffer for each image
	m_vpUniformBuffers      .resize(m_swapChainImages.size());
	m_vpUniformBuffersMemory.resize(m_swapChainImages.size());

	//m_modelDUniformBuffers      .resize(m_swapChainImages.size());
	//m_modelDUniformBuffersMemory.resize(m_swapChainImages.size());

	// Create Uniform buffers
	for (size_t i = 0; i < m_swapChainImages.size(); ++i)
	{
		CreateBuffer(m_mainDevice, vpBufferSize,
					 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					 &m_vpUniformBuffers[i], &m_vpUniformBuffersMemory[i]);

		/*
		CreateBuffer(m_mainDevice, modelBufferSize,
					 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					 &m_modelDUniformBuffers[i], &m_modelDUniformBuffersMemory[i]);
		*/
	}
}

void
VulkanRenderer::CreateDescriptorPool()
{
	// Type of descriptors that can be stored in the pool
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		// View Projection
		{
			.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,                    // Type of descriptor
			.descriptorCount = static_cast<uint32_t>(m_vpUniformBuffers.size())      // Number of descriptors (as an individual piece of data) of that type to store
		},
		/*
		 * LEGACY CODE: We are using now Push Constants, instead of Dynamic Uniform Buffers
		 *
		// Model
		{
			.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,            // Dynamic uniform buffer descriptor. Meaning: Can be updated at different offsets
			.descriptorCount = static_cast<uint32_t>(m_modelDUniformBuffers.size())
		}
		*/
	};

	// Data to create a descriptor pool
	VkDescriptorPoolCreateInfo poolCreateInfo =
	{
		.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets       = static_cast<uint32_t>(m_swapChainImages.size()),  // Maximum number of descriptor sets that can be created from the pool
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),          // Number of descriptor sets can be created from this pool
		.pPoolSizes    = poolSizes.data()                                  // Pool sizes to create the pool with
	};

	// Create Descriptor Pool
	VK_CHECK(vkCreateDescriptorPool(m_mainDevice.logicalDevice, &poolCreateInfo, nullptr, &m_descriptorPool),
			 "Failed to create a Descriptor Pool!");

	// Add to deletion queue
	m_mainDeletionQueue.push_function([&]() -> void
	{
		vkDestroyDescriptorPool(m_mainDevice.logicalDevice, m_descriptorPool, nullptr);
		m_descriptorPool = VK_NULL_HANDLE;
	});
}

void
VulkanRenderer::CreateDescriptorSets()
{
	// Resize the descriptor set list so there is one for each buffer
	m_descriptorSets.resize(m_swapChainImages.size());

	std::vector<VkDescriptorSetLayout> setLayouts(m_swapChainImages.size(), m_descriptorSetLayout);

	// Descriptor Set Allocation Info
	VkDescriptorSetAllocateInfo setAllocInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = m_descriptorPool,                                // Pool to allocate a descriptor set from
		.descriptorSetCount = static_cast<uint32_t>(m_swapChainImages.size()), // Number of sets to allocate
		.pSetLayouts        = setLayouts.data()                                // Layouts to use to allocate sets (1:1 relationship)
	};

	// Allocate descriptor sets (multiple)
	VK_CHECK(vkAllocateDescriptorSets(m_mainDevice.logicalDevice, &setAllocInfo, m_descriptorSets.data()),
			 "Failed to allocate Descriptor Sets!");

	// Update all the descriptor set buffer bindings
	for (size_t i = 0; i < m_swapChainImages.size(); ++i)
	{
		/* ----------------------- View Projection Descriptor Set ----------------------- */

		// VP Buffer descriptor
		VkDescriptorBufferInfo vpBufferInfo =
		{
			.buffer = m_vpUniformBuffers[i],   // Buffer to get data from
			.offset = 0,                       // Position of start of the data
			.range  = sizeof(ubo_view_proj_t)  // Size of data
		};

		VkWriteDescriptorSet vpSetWrite =
		{
			.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet          = m_descriptorSets[i],  // Descriptor set to update
			.dstBinding      = 0,                    // Binding to update (matches with binding on layout/shader)
			.dstArrayElement = 0,                    // Index in array to update
			.descriptorCount = 1,                    // Amount to update
			.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // Type of descriptor
			.pBufferInfo     = &vpBufferInfo          // Information about buffer data to bind
		};

		/* ----------------------- Model Descriptor Set -------------------------------- */

		/*
		 * LEGACY CODE: We are using now Push Constants, instead of Dynamic Uniform Buffers
		 *
		// Model Buffer descriptor
		VkDescriptorBufferInfo modelBufferInfo =
		{
			.buffer = m_modelDUniformBuffers[i],
			.offset = 0,
			.range  = m_modelUniformAlignment
		};

		VkWriteDescriptorSet modelSetWrite =
		{
			.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet          = m_descriptorSets[i],
			.dstBinding      = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,  // Dynamic uniform buffer descriptor
			.pBufferInfo     = nullptr
		};
		*/

		/* ----------------------- Descriptor Write Information -------------------------- */

		/// Describe the connection between a binding and a buffer.
		/// How a buffer is going to connect to a descriptor set.
		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

		vkUpdateDescriptorSets(m_mainDevice.logicalDevice,
							   static_cast<uint32_t>(setWrites.size()), setWrites.data(),
							   0, nullptr); // No copies to perform
	}
}

void
VulkanRenderer::UpdateUniformBuffers(uint32_t imageIndex)
{
	// Copy VP data
	void *data;
	vkMapMemory(m_mainDevice.logicalDevice, m_vpUniformBuffersMemory[imageIndex], 0, sizeof(ubo_view_proj_t), 0, &data);
	memcpy(data, &m_ubo_vp, sizeof(ubo_view_proj_t));
	vkUnmapMemory(m_mainDevice.logicalDevice, m_vpUniformBuffersMemory[imageIndex]);

	/*
	 * LEGACY CODE: We are using now Push Constants, instead of Dynamic Uniform Buffers
	 *
	// Copy Model data, accounting for offset alignment
	for (size_t i = 0; i < m_meshList.size(); ++i)
	{
		// Calculate the offset for the current object's model data
		const size_t modelDataOffset = i * m_modelUniformAlignment;

		// Add the offset to the base pointer
		const uint64_t modelDataPointerValue = (uint64_t)m_modelTransferSpace + modelDataOffset;

		// Update the model data in memory
		*( ( ubo_model_t * ) modelDataPointerValue ) = m_meshList[i].GetModel();
	}

	// Map the list of model data
	const size_t modelDataSize = m_meshList.size() * m_modelUniformAlignment;
	vkMapMemory(m_mainDevice.logicalDevice, m_modelDUniformBuffersMemory[imageIndex], 0, modelDataSize, 0, &data);
	memcpy(data, m_modelTransferSpace, modelDataSize);
	vkUnmapMemory(m_mainDevice.logicalDevice, m_modelDUniformBuffersMemory[imageIndex]);
	*/
}


void
VulkanRenderer::RecordCommands(VkCommandBuffer commandBuffer, uint32_t currImage)
{
	// Command buffer details
	VkCommandBufferBeginInfo bufferBeginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,                                       // Pointer to extension-related structures
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,   // Buffer can be resubmitted when it has already been submitted and is awaiting execution
		.pInheritanceInfo = nullptr                             // Secondary command buffer details
	};

	std::array<VkClearValue, 2> clearColor =
	{
		{
			//  R  |   G  |   B  |  A
			{ 0.16F, 0.16F, 0.21F, 1.0F }, // Dracula Background Colour
			//  D | S
			{ 1.0F, 0 }                    // Clear depth value to 1.0F (max value)
		}
	};

	// Info about how to begin each render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo =
	{
		.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,

		.renderPass        = m_renderPass,                               // Render Pass to begin recording commands for
		.framebuffer       = m_swapChainFramebuffers[currImage],         // Framebuffer to render to

		.renderArea        =                                             // Size of region to render to (starting point)
		{
			.offset        = { 0, 0 },                                   // Offset to start rendering at (in pixels)
			.extent        = m_swapChainExtent                           // Extent to render
		},

		.clearValueCount   = static_cast<uint32_t>(clearColor.size()),   // Number of clear values to clear the attachments
		.pClearValues      = clearColor.data()                           // Clear values to use for clear the attachments
	};

	// Framebuffer to use in the render pass
	renderPassBeginInfo.framebuffer = m_swapChainFramebuffers[currImage];


	// Start recording commands to the command buffer
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo), "Failed to start recording a command buffer");


		/* ------------------------------------- Begin the render pass -------------------------------------- */
		// Begin the render pass
		//	- VK_SUBPASS_CONTENTS_INLINE: All the render commands will be embedded in the primary command buffer. Will be inlined.
		//	- VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render commands will be executed from secondary command buffers.
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);


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
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			// ------- Scissor -------
			VkRect2D scissor = {
					.offset = {0, 0},
					.extent = m_swapChainExtent
			};
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			// ------- Bind Pipeline -------
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

			// ------- Draw -------
			for (size_t j = 0; j < m_meshList.size(); ++j)
			{
				VkBuffer vertexBuffers[] = { m_meshList[j].GetVertexBuffer() };                  // Buffers to bind
				VkDeviceSize offsets[] = { 0 };                                                  // Offsets into buffers being bound
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);       // Command to bind vertex buffer before drawing with them

				// Bind the mesh index buffer with 0 offset and using the mesh's index count
				vkCmdBindIndexBuffer(commandBuffer, m_meshList[j].GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				// Dynamic Offset Amount
				//const uint32_t dynamicOffset = static_cast<uint32_t>(m_modelUniformAlignment) * j;

				// Push constants
				//	- The data to push
				model_t model = m_meshList[j].GetModel();
				vkCmdPushConstants(
						commandBuffer,
						m_pipelineLayout,             // Pipeline layout to bind the push constants to
						VK_SHADER_STAGE_VERTEX_BIT,   // Shader stage to push constants to
						0, sizeof(model_t),           // Offset and size of data being pushed
						&model                        // Actual data being pushed (can be a struct)
				);

				// Bind the descriptor sets
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
										0, 1, &m_descriptorSets[currImage], 0, nullptr); //, 1, &dynamicOffset);

				// Execute the pipeline
				vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_meshList[j].GetIndexCount()), 1, 0, 0, 0);
			}


		/* ------------------------------------- End the render pass -------------------------------------- */
		vkCmdEndRenderPass(commandBuffer);


	// Stop recording commands to the command buffer
	VK_CHECK(vkEndCommandBuffer(commandBuffer), "Failed to stop recording a command buffer");
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

	// Get properties of the selected physical device
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(m_mainDevice.physicalDevice, &deviceProperties);

	//m_minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;
}


void
VulkanRenderer::AllocateDynamicBufferTransferSpace()
{
	/*
	 * LEGACY CODE: We are using now Push Constants, instead of Dynamic Uniform Buffers
	 *
	 // Calculate alignment of model data
	 //
	 // This alignment is necessary because the GPU may require certain types of data to start at specific memory locations.
	 // For example, a GPU may require that UBOs start at memory addresses that are multiples of 256 bytes.
	 // This is to ensure efficient memory access and optimal performance.
	 //
	 // The size of the UBO is then aligned to this requirement. The alignment is done using bitwise operations.
	 // The expression (sizeof(ubo_model_t) + uboAlignment - 1) ensures that if the size of the UBO is not a multiple of the alignment,
	 // it will round up to the next multiple of the alignment.
	 // The bitwise NOT operation ~(uboAlignment - 1) creates a mask that has all bits set to 1 from the position of the alignment bit and to the right.
	 // The bitwise AND operation & with this mask then zeroes out the lower bits of the size to make it a multiple of the alignment.
	 // This ensures that the start of each UBO falls on an address that is a multiple of the required alignment, which is necessary for the GPU to read the UBO efficiently.
	 ///
	const VkDeviceSize uboAlignment = m_minUniformBufferOffset;

	// Calculate the next multiple of the alignment after the model data size
	const size_t nextMultiple = sizeof(model_t) + (uboAlignment - 1);

	// Create a mask that zeroes out the lower bits of the size
	const size_t mask = ~(uboAlignment - 1);

	// Apply the mask to the size to align it
	m_modelUniformAlignment = (nextMultiple & mask);


	// Create space in memory to hold dynamic buffer that is at least aligned to the minimum uniform buffer offset
	m_modelTransferSpace = (model_t *)( ALIGNED_ALLOC( m_modelUniformAlignment * MAX_OBJECTS, m_modelUniformAlignment ) );

	// Add to deletion queue
	m_mainDeletionQueue.push_function([&](void) -> void
	{
		ALIGNED_FREE(m_modelTransferSpace);
	});
	*/
}


void
VulkanRenderer::CleanupSwapChain()
{
	// Clean up previous swap chain
	for (const auto &framebuffer : m_swapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_mainDevice.logicalDevice, framebuffer, nullptr);
	}
	m_swapChainFramebuffers.clear();

	for (const auto &image : m_swapChainImages)
	{
		vkDestroyImageView(m_mainDevice.logicalDevice, image.imageView, nullptr);
	}
	m_swapChainImages.clear();

	vkDestroySwapchainKHR(m_mainDevice.logicalDevice, m_swapchain, nullptr);
}

void
VulkanRenderer::CleanupFramebuffers()
{
	// Clean up uniform buffers
	for (size_t i = 0; i < m_swapChainImages.size(); ++i)
	{
		vkDestroyBuffer(m_mainDevice.logicalDevice, m_vpUniformBuffers[i], nullptr);
		vkFreeMemory(m_mainDevice.logicalDevice, m_vpUniformBuffersMemory[i], nullptr);

		//vkDestroyBuffer(m_mainDevice.logicalDevice, m_modelDUniformBuffers[i], nullptr);
		//vkFreeMemory(m_mainDevice.logicalDevice, m_modelDUniformBuffersMemory[i], nullptr);
	}

	m_vpUniformBuffers.clear();
	m_vpUniformBuffersMemory.clear();

	//m_modelDUniformBuffers.clear();
	//m_modelDUniformBuffersMemory.clear();
}

void
VulkanRenderer::CleanupDepthBuffer()
{
	// Keep in mind that the order of destruction is important
	vkDestroyImageView(m_mainDevice.logicalDevice, m_depthBufferImageView, nullptr);
	vkDestroyImage(m_mainDevice.logicalDevice, m_depthBufferImage, nullptr);
	vkFreeMemory(m_mainDevice.logicalDevice, m_depthBufferImageMemory, nullptr);
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
	/*
	// Information about the device itself (ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	 */

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

VkFormat
VulkanRenderer::ChooseSupportedFormat(const std::vector<VkFormat> &InFormats,
									  VkImageTiling tiling,
									  VkFormatFeatureFlags features) const
{
	// Loop through options and find compatible one
	for (VkFormat format : InFormats)
	{
		// Get properties for given format on this device
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(m_mainDevice.physicalDevice, format, &properties);

		// Depending on tiling choice, need to check for a different bit flag
		if ((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) ||
		    (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features))
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find a matching format!");
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

VkImage
VulkanRenderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
							VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory *imageMemory) const
{
	// ------------------------------------------------ Create Image ---------------------------------------------------

	VkImageCreateInfo imageCreateInfo =
	{
		.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext         = nullptr,                          // Pointer to extension-related structures
		.flags         = 0,                                // Optional flags
		.imageType     = VK_IMAGE_TYPE_2D,                 // Type of image (1D, 2D, or 3D)
		.format        = format,                           // Format type of the image
		.extent        =
			{
				.width = width,                            // Width of the image extent
				.height = height,                          // Height of the image extent
				.depth = 1                                 // 2D image, so depth must be 1 (no 3D aspect)
			},
		.mipLevels     = 1,                                // Number of mip levels
		.arrayLayers   = 1,                                // Number of layers (Used for multi-layer images, like stereoscopic 3D or cube maps)
		.samples       = VK_SAMPLE_COUNT_1_BIT,            // Number of samples for multi-sampling
		.tiling        = tiling,                           // How the data should be tiled
		.usage         = usage,                            // Bit flags defining what the image will be used for
		.sharingMode   = VK_SHARING_MODE_EXCLUSIVE,        // Whether the image can be shared between queues
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,        // Layout of the image data on creation
	};

	VkImage image;
	VK_CHECK(vkCreateImage(m_mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image),
			 "Failed to create an Image!");

	// ------------------------------------------------ Allocate Memory ------------------------------------------------

	// Get memory requirements for the type of image
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_mainDevice.logicalDevice, image, &memoryRequirements);

	// Allocate memory using the memory requirements and user defined properties
	VkMemoryAllocateInfo memoryAllocInfo =
	{
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize  = memoryRequirements.size,                                                                         // Size of memory allocation
		.memoryTypeIndex = FindMemoryTypeIndex(m_mainDevice.physicalDevice, memoryRequirements.memoryTypeBits, properties)  // Index of memory type on the physical device
	};

	VK_CHECK(vkAllocateMemory(m_mainDevice.logicalDevice, &memoryAllocInfo, nullptr, imageMemory),
			 "Failed to allocate memory for image!");

	// Connect memory to image
	vkBindImageMemory(m_mainDevice.logicalDevice, image, *imageMemory, 0);

	return image;
}
