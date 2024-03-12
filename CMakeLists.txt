cmake_minimum_required(VERSION 3.8)
project(Vulkan_Course LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(vendor)
add_subdirectory(src)
