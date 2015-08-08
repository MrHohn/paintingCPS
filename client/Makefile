###############################################################
#
# Purpose: Makefile for "client-OpenCV"
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
###############################################################

CC = g++

CFLAGS +=-c -g -O0 -Wall -std=c++11 -I$(MF_HOME)/common -I$(MF_HOME)/netapi/c -DLINUX

LFLAGS = -lmfapi -lpthread `pkg-config --cflags --libs opencv`

SOURCES = client-OpenCV.cpp MsgDistributor.cpp

OBJECTS = $(SOURCES:.cpp=.o)

EXECUTABLE = client-OpenCV

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LFLAGS) 

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

run:
	./client-OpenCV

debug:
	./client-OpenCV -d

orbit:
	sudo ./client-OpenCV -orbit -m 101 -o 102

odebug:
	sudo ./client-OpenCV -orbit -m 101 -o 102 -d

clean:
	rm -rf *.o $(EXECUTABLE)
