#include "clientMessenger.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>	//for uint32_t
#include <string.h>     //for memset()
#include <sys/socket.h> //for socket(), connect(), sendto(), and recvfrom()
#include <unistd.h>     //for close()
#include <netdb.h>		//for addrinfo
#include <signal.h>		//for alarm timeout

const int RESPONSE_TIMEOUT = 5;
const int RESPONSE_MESSAGE_SIZE = 1000;

//UDP socket
int sock = -1;

//private functions
void setupSocket(char* serverHost, char* serverPort);
void setupTimeoutHandler();
void timedOut(int ignored);
void* getResponseMessage(int ID, int* responseLength);
int getResponseID(char* responseMessage);
void quit(char *msg);

void setupMessenger(char* serverHost, char* serverPort) {
	setupSocket(serverHost, serverPort);
	setupTimeoutHandler();
}

/*
	Send request, receive and reassemble response
	Exit if you don't receive entire response after TIMEOUT seconds
	Set responseLength to total length of response
	Return pointer to response data
*/
void* sendRequest(char* requestString, int* responseLength) {
	//return data
	return NULL;
}

/*
	Resolve serverHost and serverPort, create socket, and connect() to server
	By connecting to the server, we don't have to specify the sockaddr_in over and over
	for every sendto(). We can use send() instead of sendto(), but send() will still
	operate in a datagram, UDP fashion. See pages 61 and 62 of TCP/IP Sockets in C
*/
void setupSocket(char* serverHost, char* serverPort) {
	struct addrinfo addrCriteria;
	memset(&addrCriteria, 0, sizeof(addrCriteria));
	addrCriteria.ai_family = AF_UNSPEC;
	addrCriteria.ai_socktype = SOCK_DGRAM;
	addrCriteria.ai_protocol = IPPROTO_UDP;

	struct addrinfo *serverAddr;
	int error = getaddrinfo(serverHost, serverPort, &addrCriteria, &serverAddr);
	if(error != 0)
		quit("could not get address information for host");

	sock = -1;
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
	
	if(sock == -1)
		quit("could not connect to host");
	
	freeaddrinfo(serverAddr);
}

void setupTimeoutHandler() {
	//create alarm and set handler
    struct sigaction handler;
    handler.sa_handler = timedOut;
    //block everything in handler
    if(sigfillset(&handler.sa_mask) < 0)
    	quit("sigfillset() failed");
    handler.sa_flags = 0;
    if(sigaction(SIGALRM, &handler, 0) < 0)
    	quit("sigaction() failed for SIGALRM");
}

//the alarm handler for the timeout alarm
void timedOut(int ignored) {
	quit("Timed out without receiving server response");
}

void quit(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}