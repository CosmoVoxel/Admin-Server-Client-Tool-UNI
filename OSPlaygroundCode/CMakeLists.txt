# Set the project name for the client
project(PlaygroundClient)

# Add the client source file
add_executable(playground priv_obf.cpp)

# Link the Winsock library
# target_link_libraries(playground ws2_32)
