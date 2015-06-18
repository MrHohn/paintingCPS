###############################################################
#
# Purpose: Makefile for "server"
# Author.: Zihong Zheng
# Version: 0.1
# License: GPL
#
###############################################################

CC = g++

# OTHER_HEADERS = ../../mjpg_streamer.h ../../utils.h ../output.h ../input.h

# CFLAGS += -O2 -DLINUX -D_GNU_SOURCE -Wall -shared -fPIC
# CFLAGS += -DDEBUG
CFLAGS += -pthread
CFLAGS += -Wall
# LFLAGS += -lpthread

all: server.o

clean:
	rm -f *.a *.o core *~ *.so *.lo

server.o: $(OTHER_HEADERS) server.cpp
	$(CC) $(CFLAGS) -o $@ server.cpp
