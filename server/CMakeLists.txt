###############################################################
#
# Purpose: CMakeList for "server-OpenCV"
# Author.: Zihong Zheng
# Version: 0.1
# License: 
#
###############################################################

cmake_minimum_required(VERSION 2.8)
project( server-OpenCV )
find_package( OpenCV REQUIRED )
add_executable( server-OpenCV server-OpenCV.cpp ImgMatch.cpp )
target_link_libraries( server-OpenCV ${OpenCV_LIBS} )
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -std=c++11")
#set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -std=c++11")
