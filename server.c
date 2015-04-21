#include "serverMessenger.h"

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

#define MAXLINE 1000 

//Method Signatures
char* getRobotID(char* msg);
uint32_t getReqID(char* msg);
char* getReq(char* msg);
char* generateReq(char* robotIP, char* robotID, char* reqStr, char* imageID);
int checkIfOverflow(char* buff, int currentSize, int amtAdded);

void serverCNTCCode();

//Main Method
int main(int argc, char *argv[])
{
	if (argc != 3)    /* Test for correct number of parameters */
	{
		fprintf(stderr,"Usage:  %s <server_port> <IP> <ID> <image_id>\n", argv[0]);
		exit(1);
	}
	
	signal(SIGINT, serverCNTCCode);
	
	//UDP Socket
	int sock;                        /* Socket */
	struct sockaddr_in echoServAddr; /* Local address */
	struct sockaddr_in echoClntAddr; /* Client address */
	unsigned int cliAddrLen; 
	char echoBuffer[MAXLINE+1];        /* Buffer for incoming */
	unsigned short echoServPort;     /* Server port */
	int recvMsgSize;                 /* Size of received message */
	
	// Robot info
	char* robotIP;
	char* robotID;
	char* imageID;
	uint32_t reqID;
	char* reqStr;
	char* request;

	//TCP Socket
	int sockfd, n;
	struct sockaddr_in servaddr;
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
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		fprintf(stderr,("socket() failed\n"));
		exit(1);
	}
		
	/* Construct local address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
	echoServAddr.sin_family = AF_INET;             /* Internet address family */
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);/* Any incoming interface */
	echoServAddr.sin_port = htons(echoServPort);      /* Local port */

	/* Bind to the local address */
	if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) 
	{
		fprintf(stderr, "bind() failed\n");
		exit(1);
	}
	
	//HANDLE UDP CLIENT
	for (;;) /* Run forever */
	{
		/* Set the size of the in-out parameter */
		cliAddrLen = sizeof(echoClntAddr);
		memset(echoBuffer, 0, strlen(echoBuffer));
		
		/* Block until receive a guess from a client */
		if ((recvMsgSize = recvfrom(sock, echoBuffer,MAXLINE, 0,
				(struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0) {
			fprintf(stderr, "recvfrom() failed\n");
			//Don't send to robot, but don't exit program
		}
		else {

			//Check Clients Request
			reqID = getReqID(echoBuffer);
			if(strcmp(robotID, getRobotID(echoBuffer)) != 0) {
				fprintf(stderr, "Robot ID's don't match\n");
				continue;
			}
			reqStr = getReq(echoBuffer);
			
			//SEND HTTP GET REQUEST
			
			//CREATE TCP SOCKET
			//Create Socket
			if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				printf("Error while creating the socket.\n");
				continue;
			}

			bzero(&servaddr, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_port   = htons(atoi(argv[2]));
			servaddr.sin_addr.s_addr = inet_addr(argv[2]);
	
			//Resolve Host
			if (servaddr.sin_addr.s_addr == -1) {
         	struct hostent *host = gethostbyname(argv[2]);
         	if (host == NULL) {
           		fprintf(stderr, "Unknown host error.\n");
	         	continue;
				}
   	      servaddr.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);
      	}
		
			//Connect
			if (connect(sockfd, (struct sockaddr *) &servaddr, 
					sizeof(servaddr)) < 0) {
				printf("Connect failed.\n");
				continue;
			}
		
			//Send HTTP Req. to Robot
			request = generateReq(robotIP, robotID, reqStr, imageID);
			if (write(sockfd, request, sizeof(request)) != sizeof(request)) {
				printf("Write error.\n");
				continue;
			}

			//Read response from Robot
			while( ( n = read(sockfd, recvline, MAXLINE)) > 0) {
				recvline[n] = 0;
				if(checkIfOverflow(responseBuffTCP, currentSize, n) == 1) {
					responseBuffTCP = realloc(responseBuffTCP, 
						sizeof(2*(currentSize+n)));
				}
				
				currentSize+= n;

				strcat(responseBuffTCP, recvline);
			}
			
			/* Send response back to the UDP client */
			sendResponse(sock, &echoClntAddr, cliAddrLen, reqID, responseBuffTCP, currentSize);

		}
	}
}

char* getRobotID(char* msg) {
	char* ptr = msg;
	ptr += sizeof(uint32_t);
	
	//go through until null ptr
	char* ptr2 = strtok(ptr, "\0");
	strcat(ptr2, "\0");

	return ptr2;
}

uint32_t getReqID(char* msg) {
	uint32_t id;
	memcpy(&id, msg, sizeof(uint32_t));
	return id;
}

char* getReq(char* msg) {
	char* ptr = msg;
	char* ptr2;
	//go through until second null ptr
	ptr2 = strstr(ptr, "\0");
	ptr2 += 1;
	return ptr2;
}

//TODO: getting number out of it for move and turn
char* generateReq(char* robotIP, char* robotID, char* reqStr, char* imageID) {
	char* request;
	request = (char*) malloc(sizeof(char)*MAXLINE);
	char* ptr;
	int n = 0;
	if(strstr(reqStr, "MOVE") != NULL) {
		ptr = strpbrk(reqStr, "0123456789");
		n = atoi(ptr);
		sprintf(request, "http://%s:8082/twist?id=%s&lx=%d",robotIP,robotID, n);
	} else if(strstr(reqStr, "TURN") != NULL) {
		ptr = strpbrk(reqStr, "0123456789");
		n = atoi(ptr);
		sprintf(request, "http://%s:8082/twist?id=%s&az=%d",robotIP,robotID,n);
	} else if(strstr(reqStr, "STOP") != NULL) {
		sprintf(request, "http://%s:8082/twist?id=%s&lx=0",robotIP,robotID);
	} else if(strstr(reqStr, "GET IMAGE") != NULL) {
		sprintf(request, "http://%s:8081/snapshot?topic=/robot_%s/image?width=600?height=500", robotIP, imageID);
	} else if(strstr(reqStr, "GET GPS") != NULL) {
		sprintf(request, "http://%s:8082/state?id=%s",robotIP,robotID);
	} else if(strstr(reqStr, "GET DGPS") != NULL) {
		sprintf(request, "http://%s:8084/state?id=%s",robotIP,robotID);
	} else if(strstr(reqStr, "GET LASERS") != NULL) {
		sprintf(request, "http://%s:8083/state?id=%s",robotIP,robotID);
	} else {
		sprintf(request, "invalid");
	}

	return request;
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

