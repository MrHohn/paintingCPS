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
add_executable( server-OpenCV server-OpenCV.cpp )
target_link_libraries( server-OpenCV ${OpenCV_LIBS} )