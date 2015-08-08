###############################################################
#
# Purpose: Makefile for "JClient"
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 0.1
# License: 
#
###############################################################


all:
	javac *.java -cp jmfapi-1.0-SNAPSHOT.jar

run:
	sudo java -cp .:jmfapi-1.0-SNAPSHOT.jar TestClient 101 102

debug:
	sudo java -cp .:jmfapi-1.0-SNAPSHOT.jar TestClient 101 102 d

clean:
	rm -rf *.class