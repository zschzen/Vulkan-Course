#ifndef VULKAN_COURSE_MESH_H
#define VULKAN_COURSE_MESH_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"


/**
 * @class Mesh
 * @brief A class to represent a mesh
 */
class Mesh
{
public:

	Mesh() = default;
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice,
		 VkQueue transferQueue, VkCommandPool transferCommandPool,
		 std::vector<vertex_t> *vertices, std::vector<uint32_t> *indices);
	~Mesh();

	/** @brief Get the number of vertices in the mesh */
	int GetVertexCount() const;

	/** @brief Get the vertex buffer */
	VkBuffer GetVertexBuffer() const;

	/** @brief Get the number of indices in the mesh */
	int GetIndexCount() const;

	/** @brief Get the index buffer */
	VkBuffer GetIndexBuffer() const;

	/** @brief Destroy the vertex buffer */
	void DestroyVertexBuffer();

private:

	// Vertex buffer
	int vertexCount                   {0}; // < The number of vertices in the mesh
	VkBuffer vertexBuffer             { }; // < The vertex buffer
	VkDeviceMemory vertexBufferMemory { }; // < The memory used to store the vertex buffer

	// Index buffer
	int indexCount                    {0}; // < The number of indices in the mesh
	VkBuffer indexBuffer              { }; // < The index buffer
	VkDeviceMemory indexBufferMemory  { }; // < The memory used to store the index buffer

	// Vulkan device
	VkPhysicalDevice physicalDevice   { }; // < The physical device used to create the vertex buffer
	VkDevice device                   { }; // < The logical device used to create the vertex buffer


	/**
	 * @brief Create the vertex buffer
	 * @param transferQueue The queue to use for transfer operations
	 * @param transferCommandPool The command pool to use for transfer operations
	 * @param vertices The vertices to create the buffer from
	 */
	void CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<vertex_t> *vertices);

	/**
	 * @brief Create the index buffer
	 * @param transferQueue The queue to use for transfer operations
	 * @param transferCommandPool The command pool to use for transfer operations
	 * @param indices The indices to create the buffer from
	 */
	void CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> *indices);
};


FORCE_INLINE int
Mesh::GetVertexCount() const
{
	return vertexCount;
}

FORCE_INLINE VkBuffer
Mesh::GetVertexBuffer() const
{
	return vertexBuffer;
}

FORCE_INLINE int
Mesh::GetIndexCount() const
{
	return indexCount;
}

FORCE_INLINE VkBuffer
Mesh::GetIndexBuffer() const
{
	return indexBuffer;
}

FORCE_INLINE void
Mesh::DestroyVertexBuffer()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);
}

#endif //VULKAN_COURSE_MESH_H
