# Set the project name for the client
project(ClientApp)

# Add the client source file
add_executable(client client.cpp
#        ../include/data.h
#        ../include/Actions.cpp
#        ../include/Actions.h
        test.cpp)

# Include json
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/json)

# target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/include)

#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/text.txt
#        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Link the Winsock library
target_link_libraries(client ws2_32)
