#ifndef UTILITIES_H
#define UTILITIES_H

#include <deque>
#include <functional>
#include <fstream>
#include <ranges>


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

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			fprintf(stderr, "[ERROR] Vulkan error: %d\n", err);     \
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
