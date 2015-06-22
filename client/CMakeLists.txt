###############################################################
#
# Purpose: CMakeList for "client-OpenCV"
# Author.: Zihong Zheng
# Version: 0.1
# License: 
#
###############################################################

cmake_minimum_required(VERSION 2.8)
project( client-OpenCV )
find_package( OpenCV REQUIRED )
add_executable( client-OpenCV client-OpenCV.cpp )
target_link_libraries( client-OpenCV ${OpenCV_LIBS} )
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -std=c++11")
#set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -std=c++11")
