# project
cmake_minimum_required(VERSION 3.1)
set(CMAKE_BUILD_TYPE RELEASE)
# Project name
project (SEATEST LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)	

# system libraries
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)

# include headers in library modules
include_directories( ${CMAKrtutils_libE_SOURCE_DIR}/../modules/rtsetup/)
include_directories( ${CMAKE_SOURCE_DIR}/../modules/GyemsCAN)
include_directories( ${CMAKE_SOURCE_DIR}/../modules/SocketCAN)
# include_directories(${EIGEN3_INCLUDE_DIRS})

# library modules needed in this project
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/rtsetup rtsetup)
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/GyemsCAN gyemscan)
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/SocketCAN socketcan)

# main function
# add_executable(main main.cpp Wheel_test.cpp Wheel_controller.cpp SEA_test.cpp SEA_controller.cpp)
add_executable(main main.cpp)

# link libraries to compile
target_link_libraries(main PUBLIC 
	rtthread_lib 
	rtutils_lib
	spdlog::spdlog
	socketcan
	gyems_can_functions
	) # needs be consistent with defination in cmakeList in sub directories
