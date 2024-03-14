#ifndef UTILITIES_H
#define UTILITIES_H

const std::vector<const char *> deviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


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

#endif //UTILITIES_H
