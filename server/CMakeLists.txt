# Set the project name for the server
project(ServerApp)

# Add the server source file
add_executable(server server.cpp)

# Link the Winsock library
target_link_libraries(server ws2_32)