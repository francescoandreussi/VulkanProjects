# Allows to set options from the CMake GUI and can condition the linking and usage of certain files
option(USE_MYMATH "Use tutorial provided math implementation" ON)

cmake_minimum_required(VERSION 3.10)

# Name and Version of the project
project(vkWarp VERSION 0.8)

# CXX Standard used
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# onfiguration of included header file
configure_file(VkWarpConfig.h.in VkWarpConfig.h)

if(USE_MYMATH)
    # Adding subdir
    # This command deos not specify its role
    # In the subdir another cmake file must be written for specifying which files have to be considered
    add_subdirectory(libs)
    list(APPEND EXTRA_LIBS libs)
    list(APPEND EXTRA_INCLUDES "${PROJECT_SOURCE_DIR}/libs")
endif()

# Signalling which executables to consider
add_executable(vkWarp simple.cpp)

# Specifying that the files considered by the CMake file in libs will be linked as libraries
target_link_libraries(vkWarp PUBLIC ${EXTRA_LIBS})

# The compilation targets will be the $Binary and $Source/libs dirs  
target_include_directories(vkWarp PUBLIC
    "${PROJECT_BINARY_DIR}"
    ${EXTRA_INCLUDES})