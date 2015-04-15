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

const int RESPONSE_TIMEOUT = 1;
const int RESPONSE_MESSAGE_SIZE = 1000;

//UDP socket
int sock = -1;

//private functions
void setupSocket(char* serverHost, char* serverPort);
void setupTimeoutHandler();
void timedOut(int ignored);
void* recvMessage(int ID, int* messageLength);
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
	static uint32_t ID = 0;
	
	//4 bytes for ID + requestString length + 1 byte for null char
	int requestLen = 5+strlen(requestString);
	void* request = malloc(requestLen);
	
	//insert ID
	*((uint32_t*)request) = htonl(ID);
	
	//insert request string
	memcpy(((char*)request)+4, requestString, strlen(requestString)+1);
	
	//send request
	int numBytesSent = send(sock, request, requestLen, 0);
	if(numBytesSent < 0)
		quit("send() failed");
	else if(numBytesSent != requestLen)
		quit("send() didn't send the whole request");

	//start timeout timer
	alarm(RESPONSE_TIMEOUT);
	
	while(true) {}
	
	//get first message so we can allocate space for all messages
	/*int responseLength;
	void* responseMessage = getResponseMessage(ID, &responseLength);
	int numMessages = getNumMessages(responseMessage);
	int sequenceNumber = getSequenceNumber(responseMessage);
	void** messages = malloc(sizeof(void*)*numMessages);
	memset(messages, 0, sizeof(void*)*numMessages);
	messages[sequenceNumber] = responseMessage;
	int numMessagesReceived = 1;
	//reassemble response messages
	while(numMessagesReceived < numMessages) {
		responseMessage = getResponseMessage(ID, &responseLength);
		sequenceNumber = getSequenceNumber(responseMessage);
		if(messages[sequenceNumber] != NULL) {
			messages[sequenceNumber] = responseMessage;
			numMessagesReceived++;
		} else {
			free(responseMessage);
		}
	}*/
	
	//update ID for next call
	ID++;
	
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