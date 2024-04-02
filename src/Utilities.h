#ifndef UTILITIES_H
#define UTILITIES_H

#include <cstdint>
#include <deque>
#include <functional>
#include <fstream>
#include <ranges>
#include <array>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <glm/glm.hpp>

#include "Checks.hpp"
#include "CommandBuffer.hpp"

// ======================================================================================================================
// ============================================ Macros ==================================================================
// ======================================================================================================================

/// Force a function to be inlined
#ifndef FORCE_INLINE
	#if defined(_MSC_VER)
		#define FORCE_INLINE __forceinline
	#elif defined(__GNUC__) || defined(__clang__)
		#define FORCE_INLINE inline __attribute__((always_inline))
	#else
		#define FORCE_INLINE inline
	#endif
#endif


/// Memory aligned allocation
#ifndef ALIGNED_ALLOC
	#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__CYGWIN__)
		#define ALIGNED_ALLOC(size, alignment) _aligned_malloc(size, alignment)
		#define ALIGNED_FREE(ptr) _aligned_free(ptr)
	#elif defined(__GNUC__)
		#define ALIGNED_ALLOC(size, alignment) aligned_alloc(alignment, size)
		#define ALIGNED_FREE(ptr) free(ptr)
	#else
		#error "Is aligned allocation supported on this platform?"
	#endif
#endif


// ======================================================================================================================
// ============================================ Vulkan Constants ========================================================
// ======================================================================================================================

/** @brief The maximum number of frames that can be in flight */
constexpr int MAX_FRAME_DRAWS = 2;

/** @brief The maximum number of objects in the scene */
constexpr int MAX_OBJECTS     = 256;

/** @brief The device extensions required by this application */
const std::vector<const char *> deviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


// ======================================================================================================================
// ============================================ Structs =================================================================
// ======================================================================================================================

/**
 * @struct device_t
 * @brief Contains the physical and logical devices
 */
typedef struct device_t
{
	VkPhysicalDevice physicalDevice {VK_NULL_HANDLE};
	VkDevice         logicalDevice  {VK_NULL_HANDLE};
} device_t;

/**
 * @struct queueFamilyIndices_t
 * @brief Contains the indices of the queue families
 * @details The indices (locations) of Queue Families (if they exist at all)
 */
typedef struct queueFamilyIndices_t
{
	// Locations
	int graphicsFamily     = -1;
	int presentationFamily = -1;

	/** @brief Check if the queue families are valid */
	inline bool IsValid() const { return graphicsFamily >= 0 && presentationFamily >= 0; }

} queueFamilyIndices_t;


/**
 * @struct swapChainDetails_t
 * @brief Contains details about the swap chain and the images used by it
 */
typedef struct swapChainDetails_t
{
	VkSurfaceCapabilitiesKHR         surfaceCapabilities {};  // Surface properties, e.g. image size/extent
	std::vector<VkSurfaceFormatKHR>  formats {};              // Surface image formats, e.g. RGBA and size of each colour
	std::vector<VkPresentModeKHR>    presentationModes {};    // How images should be presented to screen
} swapChainDetails_t;

/**
 * @struct swapchainImage_t
 * @brief Contains the image and its view (interface to the image)
 */
typedef struct swapchainImage_t
{
	VkImage     image;
	VkImageView imageView;
} swapchainImage_t;


typedef struct function_queue_t
{
	std::deque< std::function<void()> > deque {};

	/** @brief Check if the queue is empty */
	[[nodiscard]]
	FORCE_INLINE bool
	empty() const
	{
		return deque.empty();
	}

	/** @brief Get the size of the queue */
	template<typename T = size_t>
	FORCE_INLINE T
	size() const
	{
		return static_cast<T>(deque.size());
	}

	/** @brief Push a function to the queue */
	void
	push_function(std::function<void()>&& function)
	{
		deque.push_back(function);
	}

	/** @brief Flush the function queue */
	void
	flush()
	{
		for (const auto & it : std::ranges::reverse_view(deque)) (it)();
		deque.clear();
	}
} function_queue_t;

/**
 * @struct vertex_t
 * @brief Contains the position and color of a vertex
 */
typedef struct vertex_t
{
	typedef std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

	glm::vec3 pos   {};
	glm::vec3 color {};

	/** @brief Get the binding description */
	static VkVertexInputBindingDescription
	GetBindingDescription()
	{
		return VkVertexInputBindingDescription
		{
			.binding   = 0,                           // Which stream index to read from
			.stride    = sizeof(vertex_t),            // Number of bytes from one entry to the next
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX  // How to move between data entries.
													  //	VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
													  //	VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance
		};
	}

	/** @brief Get the attribute descriptions */
	static attributeDescriptions
	GetAttributeDescriptions()
	{
		return attributeDescriptions
		{
			{
				{
					.location = 0,                           // Location in the shader
					.binding  = 0,                           // Which binding the per-vertex data comes from
					.format   = VK_FORMAT_R32G32B32_SFLOAT,  // The format and size of the data
					.offset   = offsetof(vertex_t, pos)      // The number of bytes from the start of the data to read from
				},
				{
					.location = 1,
					.binding  = 0,
					.format   = VK_FORMAT_R32G32B32_SFLOAT,
					.offset   = offsetof(vertex_t, color)
				}
			}
		};
	}
} vertex_t;


// ======================================================================================================================
// ============================================ Functions ================================================================
// ======================================================================================================================

/**
 * @brief Read a file and return the contents
 *
 * @param filename The name of the file to read
 * @return The contents of the file
 */
inline std::vector<char>
ReadFile(const std::string &filename)
{
	// Open the file, and set the file pointer to the end of the file
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	// Check if the file is open
	if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filename);

	// Get the file size
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	// Go to the start of the file
	file.seekg(0);

	// Read the file into the buffer
	file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

	// Close the file and return the buffer
	file.close();
	return buffer;
}


/**
 * @brief Find the memory type index
 *
 * @param physicalDevice The physical device to use
 * @param typeFilter The memory type filter
 * @param properties The memory properties
 *
 * @return The index of the memory type
 */
[[nodiscard]]
static uint32_t
FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	// Retrieve the properties of the physical device's memory
	VkPhysicalDeviceMemoryProperties memProps {0};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

	// Iterate over the memory types available on the device
	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		// Check if the memory type is allowed
		if ( ( typeFilter & ( 1 << i ) ) == 0) continue;

		// Check if the memory type has the desired properties
		if ( ( memProps.memoryTypes[i].propertyFlags & properties ) != properties ) continue;

		// Return the index of the memory type
		return i;
	}

	throw std::runtime_error("Failed to find a suitable memory type!");
}


/**
 * @brief Create a buffer
 *
 * @param device The wrapper for the physical and logical devices
 * @param bufferSize The size of the buffer
 * @param bufferUsage The buffer usage flags
 * @param memoryProperties The memory properties
 * @param buffer The buffer to create
 * @param bufferMemory The memory to allocate to the buffer
 *
 * @return The buffer and its memory
 */
static void
CreateBuffer(const device_t &devices, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
			 VkMemoryPropertyFlags bufferProperties, VkBuffer * buffer, VkDeviceMemory * bufferMemory)
{

	/* ----------------------------------------- Create Vertex Buffer ----------------------------------------- */

	VkBufferCreateInfo bufferInfo
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = bufferSize,                             // Size of buffer (size of 1 vertex * number of vertices)
		.usage = bufferUsage,                           // Multiple types of buffer possible
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE        // Similar to Swap Chain images, can share vertex buffers
	};

	VK_CHECK(vkCreateBuffer(devices.logicalDevice, &bufferInfo, nullptr, buffer), "Failed to create a buffer!");

	/* ----------------------------------------- Get Buffer Memory Requirements ----------------------------------------- */

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(devices.logicalDevice, *buffer, &memRequirements);

	/* ----------------------------------------- Allocate Memory to Buffer ----------------------------------------- */

	// Allocate memory to the buffer
	VkMemoryAllocateInfo memoryAllocInfo
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = FindMemoryTypeIndex(devices.physicalDevice, memRequirements.memoryTypeBits,		// Index of a memory type on a Physical Device that has required bit flags
											   bufferProperties)
	};

	VK_CHECK(vkAllocateMemory(devices.logicalDevice, &memoryAllocInfo, nullptr, bufferMemory), "Failed to allocate buffer memory!");
	VK_CHECK(vkBindBufferMemory(devices.logicalDevice, *buffer, *bufferMemory, 0), "Failed to bind buffer memory!");
}

/**
 * @brief Allocate a command buffer
 *
 * @param device The logical device to use
 * @param commandPool The command pool to use
 * @param OutCommandBuffer The command buffer to allocate
 * @param bufferLevel The level of the buffer
 * @param commandBufferCount The number of command buffers to allocate
 */
static void
AllocateCommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer &OutCommandBuffer,
					  VkCommandBufferLevel bufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					  uint32_t commandBufferCount = 1)
{
	// Command buffer allocation info
	VkCommandBufferAllocateInfo allocInfo
	{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = commandPool,
		.level              = bufferLevel,
		.commandBufferCount = commandBufferCount
	};

	/**
	 *
	 * TIPS:
	 *
	 * Levels of Command Buffers:
	 *	1. Primary: Can be submitted to a queue for execution, but cannot be called from other command buffers.
	 *	2. Secondary: Cannot be submitted directly, but can be called from primary command buffers.
	 *
	 * Usage:
	 * 	vkCommandExecuteCommands(buffer) : Execute secondary command buffers from primary command buffer.
	 */

	// Allocate command buffer
	VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &OutCommandBuffer), "Failed to allocate command buffer!");
}

/**
 * @brief Copy a buffer to another buffer
 *
 * @param device The logical device to use
 * @param transferQueue The queue to use for transfer operations
 * @param transferCommandPool The command pool to use for transfer operations
 * @param srcBuffer The source buffer
 * @param dstBuffer The destination buffer
 * @param bufferSize The size of the buffer
 */
static void
CopyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		       VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
  CommandBuffer commandBuffer(device, transferCommandPool, transferQueue);

  // Region of data to copy from and to
  VkBufferCopy bufferCopyRegion
  {
	  .srcOffset = 0,          // Start at the beginning of the source buffer
	  .dstOffset = 0,          // Start at the beginning of the destination buffer
	  .size      = bufferSize  // Size of data to copy
  };

  // Command to copy src buffer to dst buffer
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);
}

/**
 * @brief Copy an image buffer
 *
 * @param device The logical device to use
 * @param transferQueue The queue to use for transfer operations
 * @param transferCommandPool The command pool to use for transfer operations
 * @param srcBuffer The source buffer
 * @param dstImage The destination image
 * @param width The width of the image
 * @param height The height of the image
 */
static void
CopyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
{
  CommandBuffer commandBuffer(device, transferCommandPool, transferQueue);

  VkBufferImageCopy imageRegion =
    {
      .bufferOffset      = 0,                           // Offset into data
      .bufferRowLength   = 0,                           // Row lenght of data to calculate data spacing
      .bufferImageHeight = 0,                           // Image height to calculate data spacing
      .imageSubresource  =
      {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,    // Wich aspect of image to copy
        .mipLevel       = 0,                            // Mipmap level to copy
        .baseArrayLayer = 0,                            // If array, starting of it
        .layerCount     = 1,                            // Number of layers to copy starting at baseArrayLayer
      },
      .imageOffset      = { 0, 0, 0 },                  // Offset into image (as opose to raw data in buffer offset)
      .imageExtent      = { width, height, 1 },         // Size of region to copy as (X, Y, Z) values
    };

    // Copy buffer to given image
    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);
}

/**
 * @brief Use memory barrier to transition image layout.
 * @details Transition image layout from one layout to another using memory barrier.
 *
 * @param device The logical device to use
 * @param queue The queue to use for the transition
 * @param commandPool The command pool to use for the transition
 * @param image The image to transition
 * @param oldLayout The old layout of the image
 * @param newLayout The new layout of the image
 */
static void
TransitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool,
                      VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
  CommandBuffer commandBuffer(device, commandPool, queue);
  /*
    * Barrier is used to synchronize access to resources, like images
    * It can be used to transfer queue family ownership, change image layout, transfer queue family ownership
    *
    * srcAccessMask: Type of access allowed from source of the image
    * dstAccessMask: Type of access to be allowed to the destination of the image
    *    Summarized: At the pont in the pipeline of srcAccessMask, this barries mus occur before dstAccessMask
    */
  VkImageMemoryBarrier imageMemoryBarrier =
  {
      .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout           = oldLayout,                 // Layout to transition from
      .newLayout           = newLayout,                 // Layout to transition to
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,   // Queue family to transition from
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,   // Queue family to transition to
      .image               = image,                     // Image to be modified as part of barrier
      .subresourceRange    =
      {
        .aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT, // Aspect of image being altered 
        .baseMipLevel      = 0,                         // First mip level to start the alteration
        .levelCount        = 1,                         // Number of mip levels to alter starting from base mip level
        .baseArrayLayer    = 0,                         // First layer to start alterations on
        .layerCount        = 1,                         // Number of layers to alter starting from baseArrayLayer
      },
  };

  VkPipelineStageFlags srcStage;
  VkPipelineStageFlags dstStage;

  // Transition the image layout
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    imageMemoryBarrier.srcAccessMask = 0;                            // Memory access stage transition must happen after this stage
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Memory access stage transition must happen before this stage

    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;                    // Top of pipeline is special stage where commands are initially processed
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;                       // Transfer stage is where transfer commands are processed
  }

  vkCmdPipelineBarrier(commandBuffer,
                        srcStage, dstStage,          // Pipeline stages (match to src and dst access masks)
                        0,                           // Dependency flags
                        0, nullptr,                  // Memory barriers count, data
                        0, nullptr,                  // Buffer memory barriers count, data
                        1, &imageMemoryBarrier);     // Image memory barriers count, data
}

#endif //UTILITIES_H
