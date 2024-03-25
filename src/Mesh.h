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
	Mesh(const device_t &devices,
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
	int m_vertexCount                   {0}; // < The number of vertices in the mesh
	VkBuffer m_vertexBuffer             { }; // < The vertex buffer
	VkDeviceMemory m_vertexBufferMemory { }; // < The memory used to store the vertex buffer

	// Index buffer
	int m_indexCount                    {0}; // < The number of indices in the mesh
	VkBuffer m_indexBuffer              { }; // < The index buffer
	VkDeviceMemory m_indexBufferMemory  { }; // < The memory used to store the index buffer

	// Vulkan device
	device_t m_devices { VK_NULL_HANDLE };


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
	return m_vertexCount;
}

FORCE_INLINE VkBuffer
Mesh::GetVertexBuffer() const
{
	return m_vertexBuffer;
}

FORCE_INLINE int
Mesh::GetIndexCount() const
{
	return m_indexCount;
}

FORCE_INLINE VkBuffer
Mesh::GetIndexBuffer() const
{
	return m_indexBuffer;
}

FORCE_INLINE void
Mesh::DestroyVertexBuffer()
{
	vkDestroyBuffer(m_devices.logicalDevice, m_vertexBuffer, nullptr);
	vkFreeMemory(m_devices.logicalDevice, m_vertexBufferMemory, nullptr);

	vkDestroyBuffer(m_devices.logicalDevice, m_indexBuffer, nullptr);
	vkFreeMemory(m_devices.logicalDevice, m_indexBufferMemory, nullptr);
}

#endif //VULKAN_COURSE_MESH_H
