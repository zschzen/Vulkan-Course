set(VULKAN_COURSE_SOURCE_FILES
        main.cpp

        Mesh.cpp
        VulkanRenderer.cpp
)

set(VULKAN_COURSE_HEADER_FILES
        Checks.hpp
        CommandBuffer.hpp
        Mesh.h
        Utilities.h
        VulkanRenderer.h
        VulkanValidation.h
)

add_executable(VulkanCourse)
target_sources(VulkanCourse PRIVATE ${VULKAN_COURSE_SOURCE_FILES} ${VULKAN_COURSE_HEADER_FILES})
target_include_directories(VulkanCourse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(VulkanCourse PRIVATE vendor)

#target_compile_definitions(VulkanCourse PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE)

set_target_properties(VulkanCourse PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set( CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" )
    set_property( DIRECTORY PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME} )
    set_property( TARGET VulkanCourse PROPERTY WIN32_EXECUTABLE true )

    target_link_options(VulkanCourse PUBLIC -static -static-libgcc -static-libstdc++)
endif ()

# Copy Assets to build directory
add_custom_command(TARGET VulkanCourse POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Assets
    $<TARGET_FILE_DIR:VulkanCourse>/Assets
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VULKAN_COURSE_SOURCE_FILES} ${VULKAN_COURSE_HEADER_FILES})
