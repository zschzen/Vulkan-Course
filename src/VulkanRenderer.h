#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <limits>
#include <vector>
#include <set>

#include "Mesh.h"
#include "Utilities.h"


/**
 * @class VulkanRenderer
 * @brief The Vulkan Renderer
 */
class VulkanRenderer
{
public:

	VulkanRenderer() = default;
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

	/** @brief Updates the model */
	void UpdateModel(uint32_t modelID, glm::mat4 newModel);

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
		app->m_framebufferResized = true;
	}

private:

	// ======================================================================================================================
	// ============================================ GLFW Components =========================================================
	// ======================================================================================================================

	/** @brief The window to render to */
	GLFWwindow *m_window  {nullptr};

	/** @brief The current frame */
	uint32_t m_currentFrame {0};

	/** @brief Is frame buffer resized */
	uint8_t m_framebufferResized : 1 {0};

	// ======================================================================================================================
	// ============================================ Scene Components ========================================================
	// ======================================================================================================================

	/** @brief The mesh list */
	std::vector<Mesh> m_meshList { };

	/** @brief Scene settings */
	struct ubo_view_proj_t
	{
		glm::mat4 proj;
		glm::mat4 view;
	} m_ubo_vp { 0 };

	// ======================================================================================================================
	// ============================================ Vulkan Components =======================================================
	// ======================================================================================================================

	// ++++++++++++++++++++++++++++++++++++++++++++++ Main Components ++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief The debug messenger callback */
	VkDebugUtilsMessengerEXT m_debugMessenger { VK_NULL_HANDLE };

	/** @brief The Vulkan instance */
	VkInstance m_instance                     { VK_NULL_HANDLE };

	/** @brief The Vulkan surface */
	VkSurfaceKHR m_surface                    { VK_NULL_HANDLE };

	VkSwapchainKHR m_swapchain                { VK_NULL_HANDLE };

	std::vector<swapchainImage_t> m_swapChainImages { };
	std::vector<VkFramebuffer> m_swapChainFramebuffers { };
	std::vector<VkCommandBuffer> m_commandBuffers { };

	// ++++++++++++++++++++++++++++++++++++++++++++++ Descriptors ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Descriptor set layout is used to define the layout of descriptors used in the pipeline.
	 * For example uniform buffers (like UBOs) and image samplers
	 */
	VkDescriptorSetLayout m_descriptorSetLayout { VK_NULL_HANDLE };

	/** @brief Push constant range */
	VkPushConstantRange m_pushConstantRange {  };

	/** @brief Descriptor pool is used to allocate descriptor sets to write descriptors into. */
	VkDescriptorPool m_descriptorPool                  { VK_NULL_HANDLE };
	std::vector<VkDescriptorSet> m_descriptorSets      {  };

	std::vector<VkBuffer>       m_vpUniformBuffers       {  };
	std::vector<VkDeviceMemory> m_vpUniformBuffersMemory {  };

	// ++++++++++++++++++++++++++++++++++++++++++++++ Dynamic Uniform Buffers +++++++++++++++++++++++++++++++++++++++++++++

	//std::vector<VkBuffer>       m_modelDUniformBuffers       {  };
	//std::vector<VkDeviceMemory> m_modelDUniformBuffersMemory {  };

	//VkDeviceSize     m_minUniformBufferOffset { 0 };
	//size_t           m_modelUniformAlignment  { 0 };
	//ubo_model_t     *m_modelTransferSpace     { nullptr };

	// +++++++++++++++++++++++++++++++++++++++++++++++ Pools +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief The command pool */
	VkCommandPool m_graphicsCommandPool { VK_NULL_HANDLE };

	// +++++++++++++++++++++++++++++++++++++++++++++++ Device Components +++++++++++++++++++++++++++++++++++++++++++++++++++

	device_t m_mainDevice { VK_NULL_HANDLE };

	// ++++++++++++++++++++++++++++++++++++++++++++++ Queues +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	// Handles to values. Don't actually hold values
	VkQueue m_graphicsQueue     { };     // Queue that handles the passing of command buffers for rendering
	VkQueue m_presentationQueue { }; // Queue that handles presentation of images to the surface

	// ++++++++++++++++++++++++++++++++++++++++++++++ Graphics Pipeline +++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief The graphics pipeline */
	VkPipeline      m_graphicsPipeline    { VK_NULL_HANDLE };

	/** @brief The pipeline layout */
	VkPipelineLayout m_pipelineLayout     { VK_NULL_HANDLE };

	/** @brief The render pass */
	VkRenderPass     m_renderPass         { VK_NULL_HANDLE };

	// ++++++++++++++++++++++++++++++++++++++++++++++ Utility Components +++++++++++++++++++++++++++++++++++++++++++++++++++

	VkFormat         m_swapChainImageFormat   {   };
	VkExtent2D       m_swapChainExtent        {   };

	function_queue_t m_mainDeletionQueue      {   };

	// ++++++++++++++++++++++++++++++++++++++++++++++ Sync Components +++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief The images available */
	std::vector<VkSemaphore> m_imageAvailableSemaphore { };

	/** @brief The max number of frames that can be processed simultaneously */
	std::vector<VkSemaphore> m_renderFinishedSemaphore { };

	/** @brief The fences */
	std::vector<VkFence> m_drawFences                  { };


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

	/** @brief Recreate the swap chain */
	void RecreateSwapChain();

	/** @brief Create the render pass */
	void CreateRenderPass();

	/** @brief Create the descriptor set layout */
	void CreateDescriptorSetLayout();

	/** @brief Create the push constant range */
	void CreatePushConstantRange();

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

	/* --------------- Descriptor Functions --------------- */

	/** @brief Create the Uniform Buffers */
	void CreateUniformBuffers();

	/** @brief Create the Descriptor Pool */
	void CreateDescriptorPool();

	/** @brief Create the Descriptor Sets */
	void CreateDescriptorSets();

	/* --------------- Uniform Buffer Functions --------------- */

	/** @brief Update the Uniform Buffers */
	void UpdateUniformBuffers(uint32_t imageIndex);

	// ++++++++++++++++++++++++++++++++++++++++++++++ Record Functions +++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief Record the command buffers */
	void RecordCommands(VkCommandBuffer commandBuffer, uint32_t currImage);

	// ++++++++++++++++++++++++++++++++++++++++++++++ Get Functions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	/**
	 * @brief Get the required extensions for the Vulkan Instance
	 * @return The required extensions for the Vulkan Instance
	 */
	std::vector<const char*> GetRequiredExtensions();

	/** @brief Get the required extensions for the Vulkan Instance */
	void GetPhysicalDevice();

	// ++++++++++++++++++++++++++++++++++++++++++++++ Allocate Functions ++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief Allocate a dynamic buffer */
	void AllocateDynamicBufferTransferSpace();

	// ++++++++++++++++++++++++++++++++++++++++++++++ Cleanup Functions +++++++++++++++++++++++++++++++++++++++++++++++++++++

	/** @brief Cleanup the swap chain */
	void CleanupSwapChain();

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
	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &InFormats);

	/**
	 * @brief Choose the best presentation mode. Subject to change
	 *
	 * @param InPresentationModes The available presentation modes
	 * @return The best presentation mode
	 */
	VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> &InPresentationModes);

	/**
	 * @brief Choose the swap extent
	 *
	 * @param InSurfaceCapabilities The surface capabilities
	 * @return The swap extent
	 */
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &InSurfaceCapabilities);

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
	[[nodiscard]] VkShaderModule CreateShaderModule(const std::vector<char> &code) const;

};


FORCE_INLINE void
VulkanRenderer::UpdateModel(uint32_t modelID, glm::mat4 newModel)
{
	if (modelID >= m_meshList.size()) return;

	m_meshList[modelID].SetModel(newModel);
}

#endif //VULKANRENDERER_H
