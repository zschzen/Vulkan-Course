#include "Mesh.h"

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice,
		   VkQueue transferQueue, VkCommandPool transferCommandPool,
		   std::vector<vertex_t> *vertices, std::vector<uint32_t> *indices)
{
	vertexCount    = static_cast<int>(vertices->size());
	indexCount     = static_cast<int>(indices->size());
	physicalDevice = newPhysicalDevice;
	device         = newDevice;

	CreateVertexBuffer(transferQueue, transferCommandPool, vertices);
	CreateIndexBuffer(transferQueue, transferCommandPool, indices);
}

Mesh::~Mesh() = default;




void
Mesh::CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<vertex_t> * vertices)
{
	VkDeviceSize bufferSize = sizeof(vertex_t) * vertices->size();

	/* ----------------------------------------- Create Staging Buffer ----------------------------------------- */

	VkBuffer stagingBuffer;                                                                   // Create a temporary buffer to stage the vertex data before transferring to the GPU
	VkDeviceMemory stagingBufferMemory;                                                       // Create the memory to allocate to the staging buffer

	CreateBuffer(physicalDevice, device, bufferSize,                                          // Create the staging buffer and allocate memory to it
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,                                            // The buffer is used as the source of a transfer operation
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  // The memory is visible to the CPU and coherent (Summarized: Can be mapped and unmapped [?] )
				 &stagingBuffer, &stagingBufferMemory);


	/* ----------------------------------------- Map Memory and Copy Data ----------------------------------------- */

	void *data;
	VK_CHECK(vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data), "Failed to map Vertex Buffer Memory!");
	memcpy(data, vertices->data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);


	/* ----------------------------------------- Create Vertex Buffer ----------------------------------------- */

	CreateBuffer(physicalDevice, device, bufferSize,
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  // The buffer is used as the destination of a transfer operation and as a vertex buffer
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                                   // The memory is only accessible by the GPU
				 &vertexBuffer, &vertexBufferMemory);

	// Copy the staging buffer to the vertex buffer
	CopyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

	// Clean up staging buffer
	vkDestroyBuffer(device, stagingBuffer, nullptr);      // Destroy the staging buffer
	vkFreeMemory(device, stagingBufferMemory, nullptr);   // Free the memory used by the staging buffer
}

void
Mesh::CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> *indices)
{
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	/* ----------------------------------------- Create Staging Buffer ----------------------------------------- */

	VkBuffer stagingBuffer;              // Create a temporary buffer to stage the index data before transferring to the GPU
	VkDeviceMemory stagingBufferMemory;  // Create the memory to allocate to the staging buffer

	CreateBuffer(physicalDevice, device, bufferSize,
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,                                           // The buffer is used as the source of a transfer operation
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // The memory is visible to the CPU and coherent (Summarized: Can be mapped and unmapped [?] )
				 &stagingBuffer, &stagingBufferMemory);


	/* ----------------------------------------- Map Memory and Copy Data ----------------------------------------- */

	void *data;
	VK_CHECK(vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data), "Failed to map Index Buffer Memory!");
	memcpy(data, indices->data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);


	/* ----------------------------------------- Create Index Buffer ----------------------------------------- */

	CreateBuffer(physicalDevice, device, bufferSize,
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,    // The buffer is used as the destination of a transfer operation and as an index buffer
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                                    // The memory is only accessible by the GPU
				 &indexBuffer, &indexBufferMemory);

	// Copy the staging buffer to the index buffer
	CopyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

	// Clean up staging buffer
	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}
