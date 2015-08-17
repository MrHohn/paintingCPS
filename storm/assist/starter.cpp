/*************************************************
#
# Purpose: Simple test program for combining CPS server and Storm cluster
# Author.: Zihong Zheng (zzhonzi@gmail.com)
# Version: 1.0
# License: 
#
*************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <sys/stat.h>

using namespace std;

int main(void)
{
	struct sockaddr_in myaddr, remaddr;
	int fd, slen = sizeof(remaddr);
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	char spoutFinderHost[100];
	sprintf(spoutFinderHost, "127.0.0.1");	/* change this to use a different server */
	char buf[1024];
	int ret;			/* # bytes received */
	int spoutFinderPort = 9877;
	int spoutPort = 9878;
	int serverPort = 9879;
	string spoutIP;
	int debug = 1;

	/* create a socket */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("socket create failed\n");
	if (debug) printf("socket created\n");

	/* bind it to all local addresses and pick any port number */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(serverPort);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       
	if (debug) printf("socket binded\n");

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(spoutFinderPort);
	if (inet_aton(spoutFinderHost, &remaddr.sin_addr) == 0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	/* now let's send the messages */

	if (debug) printf("Sending packet to %s port %d\n", spoutFinderHost, spoutFinderPort);
	sprintf(buf, "where");
	if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen) == -1)
	{
		perror("sendto");
	}

	// now receive the IP of spout
	bzero(buf, sizeof(buf));
	ret = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&remaddr, &addrlen);
	if (ret > 0)
	{
		if (strcmp(buf, "none") == 0)
		{
			printf("The spout is not ready yet.");
			close(fd);
			return 0;	
		}
		else {
			string spoutIP_temp(buf);
			spoutIP = spoutIP_temp;
			printf("Spout IP: %s\n", spoutIP.c_str());
		}
	}
	else
	{
		printf("receive error\n");		
	}
	close(fd);


	if (debug) printf("Now try to connect the spout\n");
	int sockfd;
    struct sockaddr_in spout_addr;
    struct hostent *spout;
    struct in_addr ipv4addr;
    char buf_spout[100];
    char* spout_IP;
	const int len = spoutIP.length();
	spout_IP = new char[len+1];
	strcpy(spout_IP, spoutIP.c_str());

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("ERROR opening socket\n");
    	return -1;
    }
    inet_pton(AF_INET, spout_IP, &ipv4addr);
    spout = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
	if (debug) printf("\n[server] Spout address: %s\n", spout_IP);
	if (spout == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &spout_addr, sizeof(spout_addr));
    spout_addr.sin_family = AF_INET;
    bcopy((char *)spout->h_addr, (char *)&spout_addr.sin_addr.s_addr, spout->h_length); 
    spout_addr.sin_port = htons(spoutPort);

    while (connect(sockfd,(struct sockaddr *) &spout_addr, sizeof(spout_addr)) < 0)
    {
        printf("The spout is not available now, wait a while and reconnect\n\n");
        usleep(20000); // sleep 20ms
	}

    printf("[server] Get connection to spout\n");

    char file_name[256] = "/home/hadoop/worksapce/opencv-CPS/storm/src/jvm/storm/winlab/cps/MET_IMG/IMG_1.jpg";
    // stat of file, to get the size
    struct stat file_stat;

    // get the status of file
    if (stat(file_name, &file_stat) == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    
    if (debug) printf("file size: %ld\n", file_stat.st_size);

    bzero(buf_spout, sizeof(buf_spout));
    sprintf(buf_spout, "%ld", file_stat.st_size);
    if (debug) printf("[server] send the file size\n");
    ret = write(sockfd, buf_spout, sizeof(buf_spout));
    if (ret < 0)
    {
    	printf("error sending\n");
    	return -1;
    }

    // get the response
    bzero(buf_spout, sizeof(buf_spout));
    if (debug) printf("[server] now wait for response\n");
    ret = read(sockfd, buf_spout, sizeof(buf_spout));
    if (ret < 0)
    {
    	printf("error reading\n");
    	return -1;
    }

    if (debug) printf("got response: %s\n", buf_spout);

    FILE *fp = fopen(file_name, "r");  
    if (fp == NULL)  
    {  
        printf("File:\t%s Not Found!\n", file_name);
        return -1;
    }

    char img[file_stat.st_size];
	if (debug) printf("[server] read the img\n");
    ret = fread(img, sizeof(char), file_stat.st_size, fp);
    if (ret < 0)
    {
        printf("read errro\n");
        return -1;    	
    }
	if (debug) printf("ret: %d\n", ret);

	if (debug) printf("[server] send the img\n");
    ret = write(sockfd, img, sizeof(img));
    if (ret < 0)
    {
    	printf("error sending\n");
    	return -1;
    }
	if (debug) printf("ret: %d\n", ret);
	printf("[server] Finished transmitting image to spout\n");

	close(sockfd);


	// now receive the result from bolt
	/* create a socket */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("socket create failed\n");
	if (debug) printf("socket created\n");

	/* bind it to all local addresses and pick any port number */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(serverPort);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       
	if (debug) printf("socket binded\n");

	// receive part
	bzero(buf, sizeof(buf));
	printf("wait for the result...\n");
	ret = recv(fd, buf, sizeof(buf), 0);
	if (ret > 0)
	{
		printf("received result: %s\n", buf);
	}
	else
	{
		printf("receive error\n");		
	}
	close(fd);	

	return 0;

}
