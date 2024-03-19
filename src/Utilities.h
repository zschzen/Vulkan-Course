#ifndef UTILITIES_H
#define UTILITIES_H

#include <deque>
#include <functional>
#include <fstream>
#include <ranges>

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
	[[nodiscard]] inline bool IsValid() const { return graphicsFamily >= 0 && presentationFamily >= 0; }

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
	[[nodiscard]] FORCE_INLINE bool
	empty() const
	{
		return deque.empty();
	}

	/** @brief Get the size of the queue */
	template<typename T = size_t>
	[[nodiscard]] FORCE_INLINE T
	size() const
	{
		return static_cast<T>(deque.size());
	}

	/** @brief Push a function to the queue */
	FORCE_INLINE void
	push_function(std::function<void()>&& function)
	{
		deque.push_back(function);
	}

	/** @brief Flush the function queue */
	FORCE_INLINE void
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
	[[nodiscard]] static VkVertexInputBindingDescription
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
	[[nodiscard]] static attributeDescriptions
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
[[nodiscard]]
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

#endif //UTILITIES_H
