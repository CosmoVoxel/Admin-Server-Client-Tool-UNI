cmake_minimum_required(VERSION 3.29)
project(Remote_Admin_Tool)

set(CMAKE_CXX_STANDARD 26)

# Define subdirectories for server and client
add_subdirectory(server)
add_subdirectory(client)

include_directories(${CMAKE_SOURCE_DIR}/include)