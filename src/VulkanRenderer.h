#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <limits>
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

	/** @brief Draws the frame */
	void Draw();

	/** @brief Cleans up the Vulkan Renderer */
	void Cleanup();

private:

	// ======================================================================================================================
	// ============================================ GLFW Components =========================================================
	// ======================================================================================================================

	/** @brief The window to render to */
	GLFWwindow *m_window  {nullptr};

	/** @brief The current frame */
	uint32_t m_currentFrame {0};

	// ======================================================================================================================
	// ============================================ Vulkan Components =======================================================
	// ======================================================================================================================

	// ++++++++++++++++++++++++++++++++++++++++++++++ Main Components ++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief The debug messenger callback */
	VkDebugUtilsMessengerEXT m_debugMessenger {nullptr};

	/** @brief The Vulkan instance */
	VkInstance m_instance                     {nullptr};

	/** @brief The Vulkan surface */
	VkSurfaceKHR m_surface                    {nullptr};

	VkSwapchainKHR m_swapchain                {nullptr};

	std::vector<swapchainImage_t> m_swapChainImages { };
	std::vector<VkFramebuffer> m_swapChainFramebuffers { };
	std::vector<VkCommandBuffer> m_commandBuffers { };

	// +++++++++++++++++++++++++++++++++++++++++++++++ Pools +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief The command pool */
	VkCommandPool m_graphicsCommandPool {nullptr};

	// +++++++++++++++++++++++++++++++++++++++++++++++ Device Components +++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @struct MainDevice
	 * @brief Contains information about the Vulkan device and memory
	 */
	struct
	{
		VkPhysicalDevice  physicalDevice;    // Physical device (GPU) that Vulkan will use
		VkDevice          logicalDevice;     // Logical device (application's view of the physical device) that Vulkan will use
	} m_mainDevice {};

	// ++++++++++++++++++++++++++++++++++++++++++++++ Queues +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	// Handles to values. Don't actually hold values
	VkQueue m_graphicsQueue     { };     // Queue that handles the passing of command buffers for rendering
	VkQueue m_presentationQueue { }; // Queue that handles presentation of images to the surface

	// ++++++++++++++++++++++++++++++++++++++++++++++ Graphics Pipeline +++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief The graphics pipeline */
	VkPipeline m_graphicsPipeline     {nullptr};

	/** @brief The pipeline layout */
	VkPipelineLayout m_pipelineLayout {nullptr};

	/** @brief The render pass */
	VkRenderPass m_renderPass         {nullptr};

	// ++++++++++++++++++++++++++++++++++++++++++++++ Utility Components +++++++++++++++++++++++++++++++++++++++++++++++++++

	VkFormat   m_swapChainImageFormat { };
	VkExtent2D m_swapChainExtent      { };

	// ++++++++++++++++++++++++++++++++++++++++++++++ Sync Components +++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief The images available */
	std::vector<VkSemaphore> m_imageAvailableSemaphore {nullptr};

	/** @brief The max number of frames that can be processed simultaneously */
	std::vector<VkSemaphore> m_renderFinishedSemaphore {nullptr};

	/** @brief The fences */
	std::vector<VkFence> m_drawFences {nullptr};


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

	/** @brief Create the swap chain */
	void CreateSwapChain();

	/** @brief Create the render pass */
	void CreateRenderPass();

	/** @brief Create the Graphics Pipeline */
	void CreateGraphicsPipeline();

	/** @brief Create the frame buffers */
	void CreateFramebuffers();

	/** @brief Create the command pool */
	void CreateCommandPool();

	/** @brief Create the command buffers */
	void CreateCommandBuffers();

	/** @brief Create the semaphores */
	void CreateSemaphores();

	// ++++++++++++++++++++++++++++++++++++++++++++++ Record Functions +++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief Record the command buffers */
	void RecordCommands();

	// ++++++++++++++++++++++++++++++++++++++++++++++ Get Functions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Get the required extensions for the Vulkan Instance
	 * @return The required extensions for the Vulkan Instance
	 */
	[[nodiscard]] std::vector<const char*> GetRequiredExtensions();

	/** @brief Get the required extensions for the Vulkan Instance */
	void GetPhysicalDevice();


	// ======================================================================================================================
	// ============================================ Vulkan Support Functions ================================================
	// ======================================================================================================================

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

	// ++++++++++++++++++++++++++++++++++++++++++++++ Choose Functions ++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Choose the best swap surface format. Subject to change
	 *
	 * @param InFormats The available formats
	 * @return The best surface format
	 */
	[[nodiscard]] VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &InFormats);

	/**
	 * @brief Choose the best presentation mode. Subject to change
	 *
	 * @param InPresentationModes The available presentation modes
	 * @return The best presentation mode
	 */
	[[nodiscard]] VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> &InPresentationModes);

	/**
	 * @brief Choose the swap extent
	 *
	 * @param InSurfaceCapabilities The surface capabilities
	 * @return The swap extent
	 */
	[[nodiscard]] VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &InSurfaceCapabilities);

	// ++++++++++++++++++++++++++++++++++++++++++++++++ Create Functions ++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Create an image view
	 *
	 * @param image The image to create the view for
	 * @param format The format of the image
	 * @param aspectFlags The aspect flags of the image
	 * @return The image view
	 */
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;

	/**
	 * @brief Create a shader module
	 *
	 * @param code The code of the shader
	 * @return The shader module
	 */
	VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

};

#endif //VULKANRENDERER_H
