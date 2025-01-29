# Set the project name for the server
project(ServerApp)

# Add the server source file
add_executable(server
        server.h
        server.cpp
        run.cpp
        ../include/Actions/ActionSystem.cpp
        ../include/Actions/ActionStructures.h
        ../include/Actions/Action.h
        ../include/SystemManager/OperatingSystemManager.cpp)


#include
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/include)

if (UNIX)
    target_link_libraries(server)
endif (UNIX)
if (WIN32)
    target_link_libraries(server ws2_32)
endif (WIN32)

# Проверка значения _ADMIN и настройка параметров сборки
if (_ADMIN)
    message(STATUS "Admin mode enabled. Setting up admin build.")
    target_compile_definitions(server PRIVATE _ADMIN=1)
else ()
    message(STATUS "User mode enabled. Setting up user build.")
endif ()
