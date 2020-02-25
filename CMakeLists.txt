cmake_minimum_required(VERSION 3.0.0)
project(vkWarp VERSION 0.9.0)

include(CTest)
enable_testing()

add_executable(vkWarp vkWarp/VkWarp.cpp)
include_directories(BEFORE
    "C:/VulkanSDK/Libraries/glfw-3.3.2.bin.WIN64/include/" 
    "C:/VulkanSDK/Libraries/glm/**" 
    "C:/VulkanSDK/Libraries/**" 
    "C:/VulkanSDK/1.2.131.2/Include/**")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
