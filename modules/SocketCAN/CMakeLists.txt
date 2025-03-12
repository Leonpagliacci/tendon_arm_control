# Create a library
add_library (socketcan SocketCAN.cpp)

target_include_directories(socketcan PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})