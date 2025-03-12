# Create a library
add_library (gyems_can_functions gyems.cpp gyemsManager.cpp)

target_include_directories(gyems_can_functions  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries(gyems_can_functions socketcan)
