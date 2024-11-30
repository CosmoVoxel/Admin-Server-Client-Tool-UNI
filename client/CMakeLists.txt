# Set the project name for the client
project(ClientApp)

# Add the client source file
add_executable(client
        client.cpp
        client.h
        run.cpp
        ../include/Actions/ActionSystem.cpp
        ../include/Actions/Action.h
        ../include/Networking/Networking.h
        ../include/SystemManager/OperatingSystemManager.cpp
        ../include/RequestBuilder/RequestBuilder.h
)

#include
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link the Winsock library
target_link_libraries(client ws2_32 gdiplus strmiids quartz)

