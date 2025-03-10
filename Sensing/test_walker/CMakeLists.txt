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
# find_package(Eigen3 REQUIRED)
# find_package(OpenCV REQUIRED)

# include headers in library modules
include_directories( ${CMAKrtutils_libE_SOURCE_DIR}/../modules/rtsetup/)
include_directories( ${CMAKE_SOURCE_DIR}/../modules/GyemsCAN)
include_directories( ${CMAKE_SOURCE_DIR}/../modules/SocketCAN)
include_directories( ${CMAKE_SOURCE_DIR}/../modules/Model526)
include_directories( ${CMAKE_SOURCE_DIR}/../modules/Simulink)
include_directories( ${CMAKE_SOURCE_DIR}/../modules/ATICal)
include_directories( ${CMAKE_SOURCE_DIR}/../modules/ATIDAQ)
include_directories( ${CMAKE_SOURCE_DIR}/../modules/ZLAC8030CAN)
# include_directories(${EIGEN3_INCLUDE_DIRS})

# library modules needed in this project
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/rtsetup rtsetup)
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/GyemsCAN gyemscan)
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/SocketCAN socketcan)
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/Model526 model526)
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/Simulink simulink)
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/ATICal ati_cal_lib)
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/ATIDAQ daq_lib)
add_subdirectory( ${CMAKE_SOURCE_DIR}/../modules/ZLAC8030CAN zlac8030can)

# main function
# add_executable(main main.cpp Wheel_test.cpp Wheel_controller.cpp SEA_test.cpp SEA_controller.cpp)
add_executable(main main.cpp SEA_test.cpp)

# link libraries to compile
target_link_libraries(main PUBLIC 
	rtthread_lib 
	rtutils_lib
	spdlog::spdlog
	socketcan
	zhonglings_can_functions
	gyems_can_functions
	s526_lib
	# serialPort
	simulink
	ati_cal_lib
	daq_lib
	) # needs be consistent with defination in cmakeList in sub directories
