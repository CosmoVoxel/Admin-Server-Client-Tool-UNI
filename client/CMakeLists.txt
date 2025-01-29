cmake_minimum_required(VERSION 3.10)
project(ClientApp)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the client source files
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

# Include directories
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link the libraries with coresponding system
if (UNIX)
    target_link_libraries(client)
endif (UNIX)
if (WIN32)
    target_link_libraries(client ws2_32)
endif (WIN32)

#If the admin mode is enabled, set the _ADMIN flag
if (_ADMIN)
    message(STATUS "Admin mode enabled. Setting up admin build.")
    target_compile_definitions(client PRIVATE _ADMIN=1)
else ()
    message(STATUS "User mode enabled. Setting up user build.")
endif ()
