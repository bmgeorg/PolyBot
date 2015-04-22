#include "serverMessenger.h"
#include "quit.h"

#include <stdio.h>
#include <stdlib.h>
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

#include "setupSocket.inc"

#define MAXLINE 1000 

//Method Signatures
char* getRobotID(char* msg);
uint32_t getReqID(char* msg);
char* getReq(char* msg);
char* generateReq(char* robotIP, char* robotID, char* reqStr, char* imageID);
char* getPort(char* robotCommand);
int checkIfOverflow(char* buff, int currentSize, int amtAdded);
void serverCNTCCode();

//Main Method
int main(int argc, char *argv[])
{
	if (argc != 5)    /* Test for correct number of parameters */
	{
		fprintf(stderr,"Usage:  %s <server_port> <IP> <ID> <image_id>\n", argv[0]);
		exit(1);
	}
	
	signal(SIGINT, serverCNTCCode);
	
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
	int sockRobot, n;
	char recvline[MAXLINE+1];
	char* responseBuffTCP;
	responseBuffTCP = malloc(sizeof(char)*1000);
	int currentSize = 0;	

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
			request = generateReq(robotIP, robotID, reqStr, imageID);
			if (write(sockRobot, request, sizeof(request)) != sizeof(request)) {
				printf("Write error.\n");
				continue;
			}

			//Read response from Robot
			while( ( n = read(sockRobot, recvline, MAXLINE)) > 0) {
				recvline[n] = 0;
				if(checkIfOverflow(responseBuffTCP, currentSize, n) == 1) {
					responseBuffTCP = realloc(responseBuffTCP, 
						sizeof(2*(currentSize+n)));
				}
				
				currentSize+= n;

				strcat(responseBuffTCP, recvline);
			}
			
			/* Send response back to the UDP client */
			sendResponse(sockClient, &echoClntAddr, cliAddrLen, reqID, responseBuffTCP, currentSize);

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

char* generateReq(char* robotIP, char* robotID, char* reqStr, char* imageID) {
	char* URI;
	URI = (char*) malloc(sizeof(char)*MAXLINE);
	char* ptr;
	int n = 0;
	if(strstr(reqStr, "MOVE") != NULL) {
		ptr = strpbrk(reqStr, "0123456789");
		n = atoi(ptr);
		sprintf(URI, "http://%s:8082/twist?id=%s&lx=%d",robotIP,robotID, n);
	} else if(strstr(reqStr, "TURN") != NULL) {
		ptr = strpbrk(reqStr, "0123456789");
		n = atoi(ptr);
		sprintf(URI, "http://%s:8082/twist?id=%s&az=%d",robotIP,robotID,n);
	} else if(strstr(reqStr, "STOP") != NULL) {
		sprintf(URI, "http://%s:8082/twist?id=%s&lx=0",robotIP,robotID);
	} else if(strstr(reqStr, "GET IMAGE") != NULL) {
		sprintf(URI, "http://%s:8081/snapshot?topic=/robot_%s/image?width=600?height=500", robotIP, imageID);
	} else if(strstr(reqStr, "GET GPS") != NULL) {
		sprintf(URI, "http://%s:8082/state?id=%s",robotIP,robotID);
	} else if(strstr(reqStr, "GET DGPS") != NULL) {
		sprintf(URI, "http://%s:8084/state?id=%s",robotIP,robotID);
	} else if(strstr(reqStr, "GET LASERS") != NULL) {
		sprintf(URI, "http://%s:8083/state?id=%s",robotIP,robotID);
	} else {
		sprintf(URI, "invalid");
	}

	char* req = malloc(sizeof(char)*MAXLINE);
	strcat(req, "GET ");
	strcat(req, URI);
	strcat(req, " HTTP/1.1\r\nProxy-Connection: close\r\nHost: ");
	strcat(req, robotIP);
	strcat(req, "\r\n\r\n");
	
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

//Returns 1 if overflow
int checkIfOverflow(char* buff, int currentSize, int amtAdded) {
	if( (currentSize - sizeof(buff)) <= amtAdded  ) {
		return 1;
	} else {
		return 0;
	}
}

/* This routine contains the data printing that must occur before the program 
*  quits after the CNTC signal. */
void serverCNTCCode() {
	fflush(stdout);
	exit(0);
}

