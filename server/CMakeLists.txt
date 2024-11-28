# Set the project name for the server
project(ServerApp)

# Add the server source file
add_executable(server
        server.h
        server.cpp
        run.cpp
        ../include/Actions/ActionSystem.cpp
        ../include/Actions/Action.h
        ../include/SystemManager/OperatingSystemManager.cpp)


#include
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link the Winsock library
target_link_libraries(server ws2_32)