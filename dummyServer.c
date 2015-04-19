
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>     //for memset()
#include <netinet/in.h> //for in_addr
#include <sys/socket.h> //for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>  //for sockaddr_in and inet_addr()
#include <netdb.h>      //for getHostByName()
#include <stdlib.h>     //for atoi() and exit()
#include <unistd.h>     //for close()
#include <stdbool.h>
#include <signal.h>
#include <sys/time.h>
#include <assert.h>

void quit(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <UDP Server Port>\n", argv[0]);
        exit(1);
    }
    
    unsigned short serverPort = atoi(argv[1]);

    //create socket for sending/receiving datagrams
    int sock;
    if((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        quit("socket() failed");
    }

    //construct local address structure
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(serverPort);

    //bind to the local address
    if(bind(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        quit("bind() failed");
    }
	
	while(true) {
		struct sockaddr_in clientAddress;
		unsigned int clientAddressSize = sizeof(clientAddress);
		int MAX_SIZE = 100000;
		char* msg = malloc(MAX_SIZE);
		int numReceived = recvfrom(sock, msg, MAX_SIZE, 0,
			(struct sockaddr *) &clientAddress, &clientAddressSize);
		printf("Received %d bytes\n", numReceived);
		
		uint32_t* response = malloc(1000);
		memset(response, 'x', 1000);
		response[0] = htonl(0);
		response[1] = htonl(3);
		response[2] = htonl(0);
		int numSent = sendto(sock, response, 1000, 0, (struct sockaddr *) &clientAddress, clientAddressSize);
		printf("Sent %d bytes\n", numSent);
		
		fflush(stdout);
	}
	
	return 0;
}
