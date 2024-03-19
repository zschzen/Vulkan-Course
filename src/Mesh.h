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
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<vertex_t> *vertices);
	~Mesh();

	/** @brief Get the number of vertices in the mesh */
	[[nodiscard]] int GetVertexCount() const;

	/** @brief Get the vertex buffer */
	[[nodiscard]] VkBuffer GetVertexBuffer() const;

	/** @brief Destroy the vertex buffer */
	void DestroyVertexBuffer();

private:

	int vertexCount                   {0}; // < The number of vertices in the mesh
	VkBuffer vertexBuffer             { }; // < The vertex buffer
	VkDeviceMemory vertexBufferMemory { }; // < The memory used to store the vertex buffer

	VkPhysicalDevice physicalDevice   { }; // < The physical device used to create the vertex buffer
	VkDevice device                   { }; // < The logical device used to create the vertex buffer


	/**
	 * @brief Create the vertex buffer
	 * @param vertices The vertices to create the buffer from
	 */
	void CreateVertexBuffer(std::vector<vertex_t> *vertices);

	/**
	 * @brief Find the memory type index
	 * @param allowedTypes The types of memory allowed
	 * @param properties The properties of the memory
	 * @return The index of the memory type
	 */
	[[nodiscard]] uint32_t
	FindMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties);
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

FORCE_INLINE void
Mesh::DestroyVertexBuffer()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}

#endif //VULKAN_COURSE_MESH_H
