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

	/* create a socket */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("socket create failed\n");
	printf("socket created\n");

	/* bind it to all local addresses and pick any port number */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(serverPort);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       
	printf("socket binded\n");

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

	printf("Sending packet to %s port %d\n", spoutFinderHost, spoutFinderPort);
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
		// else if (strcmp(buf, "127.0.0.1") == 0)
		// {
		// 	printf("here\n");
		// }
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


	printf("Now try to connect the spout\n");
	int sockfd;
    struct sockaddr_in spout_addr;
    struct hostent *spout;
    struct in_addr ipv4addr;
    char buf_spout[100];
    int file_size = 65536;
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
	printf("\n[server] Spout address: %s\n", spout_IP);
	if (spout == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &spout_addr, sizeof(spout_addr));
    spout_addr.sin_family = AF_INET;
    bcopy((char *)spout->h_addr, (char *)&spout_addr.sin_addr.s_addr, spout->h_length); 
    spout_addr.sin_port = htons(spoutPort);

    if (connect(sockfd,(struct sockaddr *) &spout_addr, sizeof(spout_addr)) < 0)
    {
        printf("-------- The spout is not available now. ---------\n\n");
        return -1;
    }
    else
    {
        printf("[server] Get connection to spout\n");
    }

    sprintf(buf_spout, "%d", file_size);
    printf("[server] send the file size\n");
    ret = write(sockfd, buf_spout, sizeof(buf_spout));
    if (ret < 0)
    {
    	printf("error sending\n");
    	return -1;
    }

    // get the response
    bzero(buf_spout, sizeof(buf_spout));
    printf("[server] now wait for response\n");
    ret = read(sockfd, buf_spout, sizeof(buf_spout));
    if (ret < 0)
    {
    	printf("error reading\n");
    	return -1;
    }

    printf("got response: %s\n", buf_spout);


	close(sockfd);

	return 0;

}