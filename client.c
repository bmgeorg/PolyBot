#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>     //for memset()
#include <sys/socket.h> //for socket(), connect(), sendto(), and recvfrom()
#include <unistd.h>     //for close()
#include <netdb.h>		//for addrinfo
#include <stdbool.h>

//command line arguments
char* serverHost;
char* serverPort;
double L; //L > 0
int N; //4 <= N <= 8

//other data
int sock = -1;

void quit(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

//returns true if args are valid
bool parseArgs(int argc, char** argv) {
	if(argc != 5)
		return false;
	
	serverHost = argv[1];
	serverPort = argv[2];
	if(!sscanf(argv[3], "%lf", &L) || L <= 0)
		quit("L must be greater than 0");
	N = atoi(argv[4]);
	if(N < 4 || N > 8)
		quit("N must be in the range [4, 8]");
	
	return true;
}

/*
	Resolve serverHost and serverPort, create socket, and connect() to server
	By connecting to the server, we don't have to specify the sockaddr_in over and over
	for every sendto(). We can use send() instead of sendto() (but send() will still
	operate in a datagram, UDP fashion. See pages 61 and 62 of TCP/IP Sockets in C)
*/
void setupSocket() {
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

/*
	Send request, receive and reassemble response
	Exit if you don't receive entire response after TIMEOUT seconds
	Return pointer to response data
	Return NULL if response contains no data
*/
void* sendRequest(char* requestString) {
	static uint32_t ID = 0;
	
	//4 bytes for ID +  requestString length + 1 byte for null char
	int requestLen = 5+strlen(requestString);
	void* request = malloc(requestLen);
	
	//insert ID
	*((uint32_t*)request) = htonl(ID);
	
	//insert request string
	memcpy(((char*)request)+4, requestString, strlen(requestString)+1);
	
	//send request
	int numBytesSent = send(sock, request, requestLen, 0);
	//start timeout timer
	//reassemble response
	//return data
	
	//update ID for next call
	ID++;
	
	return NULL;
}

void tracePolygon(int numSides, bool clockwise) {

}

int main(int argc, char** argv) {
	if(!parseArgs(argc, argv)) {
		fprintf(stderr, "Usage: %s <server IP or server host name> <server port> <L> <N>\n", argv[0]);
		exit(1);
	}
	setupSocket();
	tracePolygon(N, true);
	tracePolygon(N-1, false);
	
	printf("%s\n", serverHost);
	printf("%s\n", serverPort);
	printf("%f\n", L);
	printf("%d\n", N);
	printf("%d\n", sock);

	return 0;
}