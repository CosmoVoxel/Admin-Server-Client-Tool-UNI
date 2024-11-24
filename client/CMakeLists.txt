# Set the project name for the client
project(ClientApp)

# Add the client source file
add_executable(client client.cpp
        run.cpp)

# Link the Winsock library
target_link_libraries(client ws2_32 gdiplus strmiids quartz)
