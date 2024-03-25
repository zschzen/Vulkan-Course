#ifndef UTILITIES_H
#define UTILITIES_H

#include <deque>
#include <functional>
#include <fstream>
#include <ranges>
#include <array>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>


// ======================================================================================================================
// ============================================ Macros ==================================================================
// ======================================================================================================================

#ifndef FORCE_INLINE
	#if defined(_MSC_VER)
		#define FORCE_INLINE __forceinline
	#elif defined(__GNUC__) || defined(__clang__)
		#define FORCE_INLINE inline __attribute__((always_inline))
	#else
		#define FORCE_INLINE inline
	#endif
#endif

#define VK_CHECK(x, msg)               \
do                                     \
{                                      \
	VkResult err = x;                  \
	if (err != VkResult::VK_SUCCESS)   \
	{                                  \
		fprintf(stderr, "!------------ Assertion Failed ------------!\n"); \
		fprintf(stderr, "%s @ %s:%i\n", #x, __FILE__, __LINE__); \
		throw std::runtime_error(msg); \
	}                                  \
} while (0)


// ======================================================================================================================
// ============================================ Vulkan Constants ========================================================
// ======================================================================================================================

/** @brief The maximum number of frames that can be in flight */
constexpr int MAX_FRAME_DRAWS = 3;

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
 * @ brief Copy a buffer to another buffer
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
	// Allocate the command buffer
	VkCommandBuffer transferCommandBuffer {VK_NULL_HANDLE};
	AllocateCommandBuffer(device, transferCommandPool, transferCommandBuffer);

	// Begin the command buffer
	VkCommandBufferBeginInfo beginInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT  // We're only using the command buffer once
	};

	// Begin recording transfer commands
	VK_CHECK(vkBeginCommandBuffer(transferCommandBuffer, &beginInfo), "Failed to begin transfer command buffer!");

		// Region of data to copy from and to
		VkBufferCopy bufferCopyRegion
		{
			.srcOffset = 0,          // Start at the beginning of the source buffer
			.dstOffset = 0,          // Start at the beginning of the destination buffer
			.size      = bufferSize  // Size of data to copy
		};

		// Command to copy src buffer to dst buffer
		vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	// End commands
	VK_CHECK(vkEndCommandBuffer(transferCommandBuffer), "Failed to end transfer command buffer!");

	// Queue the command buffer
	VkSubmitInfo submitInfo
	{
		.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers    = &transferCommandBuffer
	};

	// Submit transfer command to transfer queue and wait until it finishes
	VK_CHECK(vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit transfer command buffer!");
	VK_CHECK(vkQueueWaitIdle(transferQueue), "Failed to wait for transfer queue to finish!");

	// Free temporary command buffer back to pool
	vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);
}

#endif //UTILITIES_H
