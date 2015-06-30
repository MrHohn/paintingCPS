###############################################################
#
# Purpose: Makefile for "server-OpenCV"
# Author.: Zihong Zheng
# Version: 0.1
# License: 
#
###############################################################

CC = g++

CFLAGS +=-c -g -O0 -Wall -std=c++11 -I$(MF_HOME)/common -I$(MF_HOME)/netapi/c/src -DLINUX

LFLAGS = -lmfapi -lpthread `pkg-config --cflags --libs opencv`

SOURCES = server-OpenCV.cpp ImgMatch.cpp

OBJECTS = $(SOURCES:.cpp=.o)

EXECUTABLE = server-OpenCV

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LFLAGS) 

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE)
