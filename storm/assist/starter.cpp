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
	int recvlen;			/* # bytes received */
	int spoutFinderPort = 9877;
	int serverPort = 9879;
	int serverPort = 9876;

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
	recvlen = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&remaddr, &addrlen);
	if (recvlen > 0)
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
			string spoutIP(buf);
			printf("Spout IP: %s\n", spoutIP.c_str());
		}
	}
	else
	{
		printf("receive error\n");		
	}

	close(fd);
	return 0;

}