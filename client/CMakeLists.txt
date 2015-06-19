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