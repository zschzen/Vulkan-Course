add_library(vendor INTERFACE)

# Vulkan
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found!")
else()
    message(STATUS "Vulkan found:")
    message(STATUS "\tVulkan_VERSION: ${Vulkan_VERSION}")
    message(STATUS "\tVulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "\tVulkan_LIBRARIES: ${Vulkan_LIBRARIES}")
endif()
target_link_libraries(vendor INTERFACE Vulkan::Vulkan)
target_include_directories(vendor INTERFACE ${Vulkan_INCLUDE_DIRS})

# GLM
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(glm)
target_link_libraries(vendor INTERFACE glm)
target_include_directories(vendor INTERFACE glm/glm)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_DOCUMENT_INTERNALS OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
target_link_libraries(vendor INTERFACE glfw)
target_include_directories(vendor INTERFACE glfw/include)

# stb
add_subdirectory(stb)
target_link_libraries(vendor INTERFACE stb)

# ImGui
add_subdirectory(imgui)
target_link_libraries(vendor INTERFACE imgui)
