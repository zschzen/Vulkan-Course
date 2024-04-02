#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Checks.hpp"

/**
 * @brief RAII wrapper for Vulkan command buffers
 *
 * @example
 * {
 *   CmdBuffer cmdBuffer(device, commandPool, queue);
 *   // Use cmdBuffer as a VkCommandBuffer
 * }
 * // cmdBuffer is automatically ended and freed when it goes out of scope
 *
 */
class CommandBuffer
{

private:
    VkDevice        device        { VK_NULL_HANDLE };
    VkCommandPool   commandPool   { VK_NULL_HANDLE };
    VkQueue         queue         { VK_NULL_HANDLE };
    VkCommandBuffer commandBuffer { VK_NULL_HANDLE };

public:

    /**
     * @brief Construct a new Command Buffer object
     *
     * @param device The Vulkan device
     * @param commandPool The command pool to allocate the command buffer from
     * @param queue The queue to submit the command buffer to
     * @param InBeginInfo The command buffer begin info (optional)
     */
    CommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBufferBeginInfo *InBeginInfo = nullptr)
        : device(device), commandPool(commandPool), queue(queue)
    {
        commandBuffer = BeginCommandBuffer(device, commandPool, InBeginInfo);
    }

    ~CommandBuffer()
    {
        EndCommandBuffer(device, commandPool, queue, commandBuffer);
    }

    // Disallow copying
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    // Allow moving
    CommandBuffer(CommandBuffer&&) = default;
    CommandBuffer& operator=(CommandBuffer&&) = default;

    operator VkCommandBuffer() const { return commandBuffer; }

private:

    /**
     * @brief Begin recording a command buffer
     *
     * @param device The Vulkan device
     * @param commandPool The command pool to allocate the command buffer from
     * @param beginInfo The command buffer begin info (optional)
     * @return The allocated command buffer
     */
    VkCommandBuffer
    BeginCommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBufferBeginInfo *beginInfo)
    {
	    // Command buffer allocation info
    	VkCommandBufferAllocateInfo allocInfo
    	{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    		.commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    		.commandBufferCount = 1
    	};

    	// Allocate command buffer
    	VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer), "Failed to allocate command buffer!");

      // Determine if we need to begin the command buffer
      if (beginInfo == nullptr)
      {
        VkCommandBufferBeginInfo defaultBeginInfo
        {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT  // Only using the command buffer once
        };
        beginInfo = &defaultBeginInfo;
      }

    	// Begin recording transfer commands
    	VK_CHECK(vkBeginCommandBuffer(commandBuffer, beginInfo), "Failed to begin transfer command buffer!");

      return commandBuffer;
    }

    /**
     * @brief End recording a command buffer and submit it
     *
     * @param device The Vulkan device
     * @param commandPool The command pool to free the command buffer back to
     * @param queue The queue to submit the command buffer to
     * @param commandBuffer The command buffer to end and submit
     */
    void
    EndCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
    {
	    VK_CHECK(vkEndCommandBuffer(commandBuffer), "Failed to end transfer command buffer!");

    	// Queue the command buffer
    	VkSubmitInfo submitInfo
    	{
		    .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    		.commandBufferCount = 1,
		    .pCommandBuffers    = &commandBuffer
    	};

    	// Submit transfer command to transfer queue and wait until it finishes
    	VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE), "Failed to submit transfer command buffer!");
    	VK_CHECK(vkQueueWaitIdle(queue), "Failed to wait for transfer queue to finish!");

    	// Free temporary command buffer back to pool
    	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
};
