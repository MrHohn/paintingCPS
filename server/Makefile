###############################################################
#
# Purpose: Makefile for "server-OpenCV"
# Author.: Zihong Zheng
# Version: 0.1
# License: 
#
###############################################################

CC = g++

CFLAGS +=-c -g -O0 -Wall -std=c++11 -I$(MF_HOME)/common -I$(MF_HOME)/netapi/c -DLINUX -I/usr/local/include/librdkafka

LFLAGS = -lmfapi -lz -lpthread -lrt `pkg-config --cflags --libs opencv` -lcrypto -lrdkafka++ -lrdkafka

SOURCES = server-OpenCV.cpp ImgMatch.cpp MsgDistributor.cpp KafkaProducer.cpp

OBJECTS = $(SOURCES:.cpp=.o)

EXECUTABLE = server-OpenCV

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LFLAGS) 

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

run:
	./server-OpenCV

debug:
	./server-OpenCV -d

orbit:
	sudo ./server-OpenCV -orbit -m 102 -o 101

odebug:
	sudo ./server-OpenCV -orbit -m 102 -o 101 -d

clean:
	rm -rf *.o $(EXECUTABLE)
