#include "Mesh.h"

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<vertex_t> *vertices)
{
	vertexCount = static_cast<int>(vertices->size());
	physicalDevice = newPhysicalDevice;
	device = newDevice;
	CreateVertexBuffer(vertices);
}

Mesh::~Mesh() = default;




void
Mesh::CreateVertexBuffer(std::vector<vertex_t> *vertices)
{
	/* ----------------------------------------- Create Vertex Buffer ----------------------------------------- */

	VkBufferCreateInfo bufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = sizeof(vertex_t) * vertices->size(),    // Size of buffer (size of 1 vertex * number of vertices)
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,     // Multiple types of buffer possible, we want Vertex Buffer
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE        // Similar to Swap Chain images, can share vertex buffers
	};

	VK_CHECK(vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer), "Failed to create a Vertex Buffer!");

	/* ----------------------------------------- Get Buffer Memory Requirements ----------------------------------------- */

	VkMemoryRequirements memRequirements {0};
	vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

	/* ----------------------------------------- Allocate Memory to Buffer ----------------------------------------- */

	VkMemoryAllocateInfo memoryAllocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = FindMemoryTypeIndex(memRequirements.memoryTypeBits,                                              // Index of a memory type to use
											   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)  // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: Memory is mappable by host
													                                                                        // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: CPU writes are automatically visible to the GPU
	};

	// Allocate memory to the vertex buffer and bind it
	VK_CHECK(vkAllocateMemory(device, &memoryAllocInfo, nullptr, &vertexBufferMemory), "Failed to allocate Vertex Buffer Memory!");
	VK_CHECK(vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0), "Failed to bind Vertex Buffer Memory!");

	/* ----------------------------------------- Map Memory and Copy Data ----------------------------------------- */

	void *data;                                                                      // Create a pointer to a point in memory
	VK_CHECK(vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data),
			 "Failed to map Vertex Buffer Memory!");                                 // Map the vertex buffer memory to the pointer
	memcpy(data, vertices->data(), static_cast<size_t>(bufferInfo.size));            // Copy the vertex data to the pointer
	vkUnmapMemory(device, vertexBufferMemory);                                       // Unmap the vertex buffer memory
}

uint32_t
Mesh::FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags desiredProperties)
{
	// Retrieve the properties of the physical device's memory
	VkPhysicalDeviceMemoryProperties memProps {0};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

	// Iterate over the memory types available on the device
	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		// Check if the memory type is allowed
		if ( ( allowedTypes & ( 1 << i ) ) == 0) continue;

		// Check if the memory type has the desired properties
		if ( ( memProps.memoryTypes[i].propertyFlags & desiredProperties ) != desiredProperties ) continue;

		// Return the index of the memory type
		return i;
	}

	throw std::runtime_error("Failed to find a suitable memory type!");
}
