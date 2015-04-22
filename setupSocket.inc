#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <netdb.h>
#include <unistd.h>     /* for close() */
#include <string.h>		/* for memset() */

typedef enum {TCP, UDP} SocketType;

//assume TCP unless isUDP is true
int setupSocket(char* serverHost, char* serverPort, SocketType type) {
	struct addrinfo addrCriteria;
	memset(&addrCriteria, 0, sizeof(addrCriteria));
	addrCriteria.ai_family = AF_INET;
	if(type == UDP) {
		addrCriteria.ai_socktype = SOCK_DGRAM;
		addrCriteria.ai_protocol = IPPROTO_UDP;
	}
	else if(type == TCP) {
		addrCriteria.ai_socktype = SOCK_STREAM;
		addrCriteria.ai_protocol = IPPROTO_TCP;
	}
	else {
		fprintf(stderr, "unexpected SocketType in setupSocket()\n");
		return -1;
	}
	struct addrinfo *serverAddr;
	int error = getaddrinfo(serverHost, serverPort, &addrCriteria, &serverAddr);
	if(error != 0) {
		fprintf(stderr, "could not get address information for host\n");
		return -1;
	}

	int sock = -1;
	//loop through addresses and try to connect() to each one
	struct addrinfo* addr;
	for(addr = serverAddr; addr != NULL; addr = addr->ai_next) {
		sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if(sock < 0) {
			//socket creation failed -- try next address
			continue;
		}
		if(connect(sock, addr->ai_addr, addr->ai_addrlen) == 0) {
			//socket connection succeeded
			break;
		}
		//socket connection failed -- try next address
		close(sock);
		sock = -1;
	}

	freeaddrinfo(serverAddr);
	
	return sock;
}