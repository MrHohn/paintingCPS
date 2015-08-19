CC = g++

CFLAGS +=-c -g -O0 -Wall -fPIC -std=c++11 -I/usr/lib/jvm/java-8-oracle/include -I/usr/lib/jvm/java-8-oracle/include/linux -I/usr/lib/jvm/java-7-openjdk-i386/include -I/usr/lib/jvm/java-7-openjdk-i386/include/linux -I/usr/include -DLINUX 

LFLAGS = `pkg-config --cflags --libs opencv`
LDFLAGS = -shared

SOURCES = JniImageMatching.cpp ImgMatch.cpp

OBJECTS = $(SOURCES:.cpp=.o)

OUTPUT = libJniImageMatching.so

all: $(OUTPUT) $(SOURCES)
	
$(OUTPUT):$(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LFLAGS)
.cpp.o:
	$(CC) -c $(CFLAGS) $< -o $*.o

clean:
	rm -rf *.o $(OUTPUT)
