# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.4)

# Project's name, version, and languages
project(c-chat VERSION 1.0 LANGUAGES C)

# Set the output folder where the executables will be created
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/../bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Directory containing the header included
include_directories(${CMAKE_SOURCE_DIR}/util)

# Add the subdirectories where the source files are present
add_subdirectory(util)
add_subdirectory(client)
add_subdirectory(server)
