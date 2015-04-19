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

//Global Variables 

//Method Signatures
char* getRobotID(char* msg);
uint32_t getReqID(char* msg);
char* getReq(char* msg);
char* generateReq(char* robotIP, char* robotID, char* reqStr, char* imageID);

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
	unsigned int cliAddrLen;         /* Length of incoming message */
	char echoBuffer[MAXLINE+1];        /* Buffer for guess */
	char returnBuffer[MAXLINE+1];      /* Buffer for returnCode */
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
	char* buffer;
	size_t result;
	int sockfd, n;
	int sendbytes;
	struct sockaddr_in servaddr;
	char sendline[MAXLINE+1];
	char recvline[MAXLINE+1];

	echoServPort = atoi(argv[1]); 
	robotIP = argv[2];
	robotID = argv[3];
	imageID = argv[4];

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
	
	//Handle UDP Client
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
			if(strcmp(robotID, getRobotID(echoBuffer)) != 0) {
				fprintf(stderr, "Robot ID's don't match\n");
				continue;
			}
			reqID = getReqID(echoBuffer);
			reqStr = getReq(echoBuffer);
			
			if(sendto(sock, returnBuffer, strlen(returnBuffer), 0, 
					(struct sockaddr *) &echoClntAddr, 
					sizeof(echoClntAddr)) != sizeof(returnBuffer)) {
				fprintf(stderr,"sendto() sent a different number of bytes than expected\n");
				continue;
			}

			//SEND HTTP GET REQUEST

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
		
			//Send
			request = generateReq(robotIP, robotID, reqStr, imageID);
			if (write(sockfd, request, sizeof(request)) != sizeof(request)) {
				printf("Write error.\n");
				continue;
			}

			//Read response
			while( ( n = read(sockfd, recvline, MAXLINE)) > 0) {
				recvline[n] = 0;
				//SHOULD OUTPUT TO MEM CHUNK AND THEN SEND TODO
				if(fputs(recvline, stdout) == EOF) {
					fprintf(stderr, "fputs error");
					continue;
				}
			}

			/* Send response back to the client */
			//Brett's function

		}
	}
}

char* getRobotID(char* msg) {
	
}

uint32_t getReqID(char* msg) {

}

char* getReq(char* msg) {
	
}

//TODO: getting number out of it for move and turn
char* generateReq(char* robotIP, char* robotID, char* reqStr, char* imageID) {
	char* request;
	request = (char*) malloc(sizeof(char)*MAXLINE);
	
	int n = 0;
	if(strstr(reqStr, "MOVE") != NULL) {
		sprintf(request, "http://%s:8082/twist?id=%s&lx=%d\0",robotIP,robotID, n);
	} else if(strstr(reqStr, "TURN") != NULL) {
		sprintf(request, "http://%s:8082/twist?id=%s&az=%d\0",robotIP,robotID,n);
	} else if(strstr(reqStr, "STOP") != NULL) {
		sprintf(request, "http://%s:8082/twist?id=%s&lx=0\0",robotIP,robotID);
	} else if(strstr(reqStr, "GET IMAGE") != NULL) {
		sprintf(request, "http://%s:8081/snapshot?topic=/robot_%s/image?width=600?height=500", robotIP, imageID);
	} else if(strstr(reqStr, "GET GPS") != NULL) {
		sprintf(request, "http://%s:8082/state?id=%s\0",robotIP,robotID);
	} else if(strstr(reqStr, "GET DGPS") != NULL) {
		sprintf(request, "http://%s:8084/state?id=%s\0",robotIP,robotID);
	} else if(strstr(reqStr, "GET LASERS") != NULL) {
		sprintf(request, "http://%s:8083/state?id=%s\0",robotIP,robotID);
	} else {
		sprintf(request, "invalid\0");
	}

	return request;
}

/* This routine contains the data printing that must occur before the program 
*  quits after the CNTC signal. */
void serverCNTCCode() {
	fflush(stdout);
	exit(0);
}

