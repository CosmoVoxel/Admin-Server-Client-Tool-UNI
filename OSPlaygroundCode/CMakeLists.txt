# Set the project name for the client
project(PlaygroundClient)

# Add the client source file
add_executable(playground priv_obf.cpp ../include/SystemManager/OperatingSystemManager.cpp)
