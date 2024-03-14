set(VULKAN_COURSE_SOURCE_FILES
        main.cpp

        VulkanRenderer.cpp
)

set(VULKAN_COURSE_HEADER_FILES
        Utilities.h
        VulkanRenderer.h
        VulkanValidation.h
)

add_executable(VulkanCourse)
target_sources(VulkanCourse PRIVATE ${VULKAN_COURSE_SOURCE_FILES} ${VULKAN_COURSE_HEADER_FILES})
target_include_directories(VulkanCourse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(VulkanCourse PRIVATE vendor)

set_target_properties(VulkanCourse PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VULKAN_COURSE_SOURCE_FILES} ${VULKAN_COURSE_HEADER_FILES})
