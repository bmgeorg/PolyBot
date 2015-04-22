#include "serverMessenger.h"
#include "utility.h"
#include "setupClientSocket.inc"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>     /* for memset() */
#include <netinet/in.h> /* for in_addr */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <netdb.h>      /* for getHostByName() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <unistd.h>     /* for close() */
#include <time.h>       /* for time() */
#include <signal.h>

#define MAXLINE 1000 

//Method Signatures
char* getRobotID(char* msg);
uint32_t getRequestID(char* msg);
char* getRequestStr(char* msg);
char* generateHTTPRequest(char* robotAddress, char* robotID, char* requestStr, char* imageID);
char* getRobotPortForRequestStr(char* requestStr);
void flushBuffersAndExit();

//Main Method
int main(int argc, char *argv[])
{
	if(argc != 5) {
		quit("Usage:  %s <server_port> <robot_IP/robot_hostname> <robot_ID> <image_id>", argv[0]);
	}
	//read args
	unsigned short localUDPPort = atoi(argv[1]);
	char* robotAddress = argv[2];
	char* robotID = argv[3];
	char* imageID = argv[4];
	
	printf("Read arguments\n");
	printf("Robot address: %s\n", robotAddress);
	printf("Robot ID: %s\n", robotID);
	printf("Image ID: %s\n", imageID);
	
	//listen for ctrl-c and call flushBuffersAndExit()
	signal(SIGINT, flushBuffersAndExit);

	//Create socket for talking to clients
	int clientSock;
	if((clientSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		quit("could not create client socket - socket() failed");
	}
		
	//Construct local address structure for talking to clients
	struct sockaddr_in localAddress;
	memset(&localAddress, 0, sizeof(localAddress));		//Zero out structure
	localAddress.sin_family = AF_INET;					//Any Internet address family
	localAddress.sin_addr.s_addr = htonl(INADDR_ANY);	//Any incoming interface
	localAddress.sin_port = htons(localUDPPort);		//The port clients will sendto

	if(bind(clientSock, (struct sockaddr *) &localAddress, sizeof(localAddress)) < 0) {
		quit("could not bind to client socket - bind() failed");
	}

	//Loop for each client request
	for(;;) {
		//Receive request from client
		struct sockaddr_in clientAddress;
		unsigned int clientAddressLen = sizeof(clientAddress);	//in-out parameter
		char clientBuffer[MAXLINE+1];	//Buffer for incoming client requests
		memset(clientBuffer, 0, MAXLINE+1);
		
		int recvMsgSize;
		//Block until receive a guess from a client
		if((recvMsgSize = recvfrom(clientSock, clientBuffer, MAXLINE, 0,
				(struct sockaddr *) &clientAddress, &clientAddressLen)) < 0) {
			quit("could not receive client request - recvfrom() failed");
		}

		//Interpret client request
		char* requestRobotID = getRobotID(clientBuffer);
		if(strcmp(robotID, requestRobotID) != 0) {
			fprintf(stderr, "invalid request - robot ID's don't match\n");
			continue;
		}

		char* requestStr = getRequestStr(clientBuffer);
		char* robotPort = getRobotPortForRequestStr(requestStr);
		//Send HTTP request to robot
		int robotSock;
		if((robotSock = setupClientSocket(robotAddress, robotPort, SOCKET_TYPE_TCP)) < 0) {
			quit("could not connect to robot");
		}	
		char* httpRequest = generateHTTPRequest(robotAddress, robotID, requestStr, imageID);
		if(write(robotSock, httpRequest, strlen(httpRequest)) != strlen(httpRequest)) {
			quit("could not send http request to robot - write() failed");
		}

		//Read response from Robot
		int pos = 0;
		char* httpResponse = malloc(MAXLINE);
		int n;
		char recvLine[MAXLINE+1]; //holds one chunk of read data at a time
		while((n = read(robotSock, recvLine, MAXLINE)) > 0) {
			memcpy(httpResponse+pos, recvLine, n);
			pos += n;
			httpResponse = realloc(httpResponse, pos+MAXLINE);
		}
		printf("Response:\n%s", httpResponse);
		
		//Parse Response from Robot
		char* httpBody = strstr(httpResponse, "\r\n\r\n")+4;
		int httpBodyLength = (httpResponse+pos)-httpBody;
		printf("Response length: %d\n", httpBodyLength);
		
		//Send response back to the UDP client
		uint32_t requestID = getRequestID(clientBuffer);
		sendResponse(clientSock, &clientAddress, clientAddressLen, requestID, httpBody, httpBodyLength);
		
		free(httpResponse);
	}
}

char* getRobotID(char* msg) {
	return msg+4;
}

uint32_t getRequestID(char* msg) {
	return ntohl(*((uint32_t*)msg));
}

char* getRequestStr(char* msg) {
	char* robotID = getRobotID(msg);

	return msg+5+strlen(robotID);
}

char* generateHTTPRequest(char* robotAddress, char* robotID, char* requestStr, char* imageID) {
	char* URI = malloc(MAXLINE);
	double x;
	if(sscanf(requestStr, "MOVE %lf", &x) == 1) {
		sprintf(URI, "/twist?id=%s&lx=%lf", robotID, x);
	}
	else if(sscanf(requestStr, "TURN %lf", &x) == 1) {
		sprintf(URI, "/twist?id=%s&az=%lf", robotID, x);
	}
	else if(strstr(requestStr, "STOP") != NULL) {
		sprintf(URI, "/twist?id=%s&lx=0",robotID);
	}
	else if(strstr(requestStr, "GET IMAGE") != NULL) {
		sprintf(URI, "/snapshot?topic=/robot_%s/image?width=600?height=500", imageID);
	}
	else if(strstr(requestStr, "GET GPS") != NULL) {
		sprintf(URI, "/state?id=%s",robotID);
	}
	else if(strstr(requestStr, "GET DGPS") != NULL) {
		sprintf(URI, "/state?id=%s",robotID);
	}
	else if(strstr(requestStr, "GET LASERS") != NULL) {
		sprintf(URI, "/state?id=%s",robotID);
	}
	else {
		free(URI);
		return NULL;
	}

	char* httpRequest = malloc(MAXLINE);
	strcat(httpRequest, "GET ");
	strcat(httpRequest, URI);
	strcat(httpRequest, " HTTP/1.1\r\n");
	
	//Host
	strcat(httpRequest, "Host: ");
	strcat(httpRequest, robotAddress);
	strcat(httpRequest, ":");
	strcat(httpRequest, getRobotPortForRequestStr(requestStr));
	strcat(httpRequest, "\r\n");
	
	//Connection: close
	strcat(httpRequest, "Connection: close\r\n");
	
	strcat(httpRequest, "\r\n");
	
	fprintf(stderr, "HTTP Request:\n%s", httpRequest);	

	free(URI);
	return httpRequest;
}

//Gets TCP Port
char* getRobotPortForRequestStr(char* requestStr) {
	if(strstr(requestStr, "MOVE") != NULL) {
		return "8082";
	} else if(strstr(requestStr, "TURN") != NULL) {
		return "8082";
	} else if(strstr(requestStr, "STOP") != NULL) {
		return "8082";
	} else if(strstr(requestStr, "GET IMAGE") != NULL) {
		return "8081";
	} else if(strstr(requestStr, "GET GPS") != NULL) {
		return "8082";
	} else if(strstr(requestStr, "GET DGPS") != NULL) {
		return "8084";
	} else if(strstr(requestStr, "GET LASERS") != NULL) {
		return "8083";
	} else {
		return NULL;
	}
}

/* This routine contains the data printing that must occur before the program 
*  quits after the CNTC signal. */
void flushBuffersAndExit() {
	fflush(stdout);
	exit(0);
}

