#include "serverMessenger.h"
#include "quit.h"
#include "setupSocket.inc"

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
uint32_t getReqID(char* msg);
char* getReq(char* msg);
char* generateHTTPReq(char* robotIP, char* robotID, char* reqStr, char* imageID);
char* getPort(char* robotCommand);
void flushBuffersAndExit();

//Main Method
int main(int argc, char *argv[])
{
	if (argc != 5) {
		fprintf(stderr,"Usage:  %s <server_port> <IP> <ID> <image_id>\n", argv[0]);
		exit(1);
	}
	
	signal(SIGINT, flushBuffersAndExit);
	
	//UDP Socket Variables
	int sockClient;                        /* Socket */
	struct sockaddr_in echoServAddr; /* Local address */
	struct sockaddr_in echoClntAddr; /* Client address */
	unsigned int cliAddrLen; 
	char clientBuffer[MAXLINE+1];        /* Buffer for incoming */
	unsigned short echoServPort;     /* Server port */
	int recvMsgSize;                 /* Size of received message */
	
	// Robot Variables
	char* robotIP;
	char* robotID;
	char* imageID;
	uint32_t reqID;
	char* reqStr;
	char* request;

	//TCP Socket Variables
	int sockRobot;
	char* responseBuffTCP;
	responseBuffTCP = malloc(sizeof(char)*1000);

	echoServPort = atoi(argv[1]); 
	robotIP = argv[2];
	robotID = argv[3];
	imageID = argv[4];

	//CREATE UDP SOCKET
	/* Create socket for sending/receiving datagrams */
	if ((sockClient = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		fprintf(stderr,("socket() failed\n"));
		exit(1);
	}
		
	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
	echoServAddr.sin_family = AF_INET;             /* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);/* Any incoming interface */
	echoServAddr.sin_port = htons(echoServPort);      /* Local port */

	/* Bind to the local address */
	if (bind(sockClient, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) 
	{
		fprintf(stderr, "bind() failed\n");
		exit(1);
	}

	//HANDLE UDP CLIENT
	for (;;) /* Run forever */
	{
		/* Set the size of the in-out parameter */
		cliAddrLen = sizeof(echoClntAddr);
		memset(clientBuffer, 0, strlen(clientBuffer));
		
		/* Block until receive a guess from a client */
		if ((recvMsgSize = recvfrom(sockClient, clientBuffer,MAXLINE, 0,
				(struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0) {
			fprintf(stderr, "recvfrom() failed\n");
			//Don't send to robot, but don't exit program
		}
		else {
			//Check Clients Request
			reqID = getReqID(clientBuffer);
			if(strcmp(robotID, getRobotID(clientBuffer)) != 0) {
				fprintf(stderr, "Robot ID's don't match\n");
				continue;
			}

			reqStr = NULL;
			reqStr = getReq(clientBuffer);
			
			//SEND HTTP GET REQUEST
			
			sockRobot = setupSocket(robotIP, getPort(reqStr), TCP);
		
			//Send HTTP Req. to Robot
			request = generateHTTPReq(robotIP, robotID, reqStr, imageID);
			if(write(sockRobot, request, strlen(request)) != strlen(request)) {
				fprintf(stderr, "Write error.\n");
				continue;
			}

			//Read response from Robot
			int n;
			int pos = 0;
			char recvline[MAXLINE+1];
			while( (n = read(sockRobot, recvline, MAXLINE)) > 0) {
				printf("loop");
				fflush(stdout);
				memcpy(responseBuffTCP+pos, recvline, n);
				pos += n;
				responseBuffTCP = realloc(responseBuffTCP, pos+MAXLINE);
				printf("postloop");
				fflush(stdout);
			}
			printf("Response:\n%s", responseBuffTCP);
			
			//Parse Response from Robot
			char* end = strstr(responseBuffTCP, "\r\n\r\n")+4;
			int responseLength = (responseBuffTCP+pos)-end;
			printf("Response length: %d\n", responseLength);
			/* Send response back to the UDP client */
			sendResponse(sockClient, &echoClntAddr, cliAddrLen, reqID, end, responseLength);
		}
	}
	
	free(responseBuffTCP);
	free(request);
}

char* getRobotID(char* msg) {
	return msg+4;
}

uint32_t getReqID(char* msg) {
	return ntohl(*((uint32_t*)msg));
}

char* getReq(char* msg) {
	char* robotID = getRobotID(msg);

	return msg+5+strlen(robotID);
}

char* generateHTTPReq(char* robotIP, char* robotID, char* reqStr, char* imageID) {
	char* URI;
	URI = (char*) malloc(sizeof(char)*MAXLINE);
	char* ptr;
	int n = 0;
	if(strstr(reqStr, "MOVE") != NULL) {
		ptr = strpbrk(reqStr, "0123456789");
		n = atoi(ptr);
		sprintf(URI, "/twist?id=%s&lx=%d", robotID, n);
	} else if(strstr(reqStr, "TURN") != NULL) {
		ptr = strpbrk(reqStr, "0123456789");
		n = atoi(ptr);
		sprintf(URI, "/twist?id=%s&az=%d",robotID,n);
	} else if(strstr(reqStr, "STOP") != NULL) {
		sprintf(URI, "/twist?id=%s&lx=0",robotID);
	} else if(strstr(reqStr, "GET IMAGE") != NULL) {
		sprintf(URI, "/snapshot?topic=/robot_%s/image?width=600?height=500", imageID);
	} else if(strstr(reqStr, "GET GPS") != NULL) {
		sprintf(URI, "/state?id=%s",robotID);
	} else if(strstr(reqStr, "GET DGPS") != NULL) {
		sprintf(URI, "/state?id=%s",robotID);
	} else if(strstr(reqStr, "GET LASERS") != NULL) {
		sprintf(URI, "/state?id=%s",robotID);
	} else {
		sprintf(URI, "invalid");
	}

	char* req = malloc(sizeof(char)*MAXLINE);
	strcat(req, "GET ");
	strcat(req, URI);
	strcat(req, " HTTP/1.1\r\n");
	
	//Host
	strcat(req, "Host: ");
	strcat(req, robotIP);
	strcat(req, ":");
	strcat(req, getPort(reqStr));
	strcat(req, "\r\n");
	
	//Connection: close
	strcat(req, "Connection: close\r\n");
	
	strcat(req, "\r\n");
	
	fprintf(stderr, "Request:\n%s", req);	

	free(URI);
	return req;
}

//Gets TCP Port
char* getPort(char* reqStr) {
	
	if(strstr(reqStr, "MOVE") != NULL) {
		return "8082";
	} else if(strstr(reqStr, "TURN") != NULL) {
		return "8082";
	} else if(strstr(reqStr, "STOP") != NULL) {
		return "8082";
	} else if(strstr(reqStr, "GET IMAGE") != NULL) {
		return "8081";
	} else if(strstr(reqStr, "GET GPS") != NULL) {
		return "8082";
	} else if(strstr(reqStr, "GET DGPS") != NULL) {
		return "8084";
	} else if(strstr(reqStr, "GET LASERS") != NULL) {
		return "8083";
	} else {
		return "0";
	}
	
}

/* This routine contains the data printing that must occur before the program 
*  quits after the CNTC signal. */
void flushBuffersAndExit() {
	fflush(stdout);
	exit(0);
}

