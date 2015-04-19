#include "clientMessenger.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void tracePolygon(int numSides, bool clockwise);
void getSnapshot();

double L; //L > 0
int N; //4 <= N <= 8
int fileCount = 0;

void tracePolygon(int numSides, bool clockwise) {
   int dummy;
   char *moveRequest = (char *)malloc(50);
   char *turnRequest = (char *)malloc(50);
   char *stopMoveRequest = "MOVE 0";
   char *stopTurnRequest = "TURN 0";
   
   
   //Take initial screenshot before making the polygon.
   getSnapshot();

   int i;
   for(i = 0; i < numSides; i++) {
      sendRequest(
   }

}

void getSnapshot() {
   int length;
   char *data;

   FILE *imageFile;       //File for the image data received.
   FILE *positionFile;    //File for the position data received.

   char *imageFileName = (char *)malloc(50);
   char *positionFileName = (char *)malloc(50);

   sprintf(imageFileName, "image-%d.png", fileCount);
   sprintf(positionFileName, "position-%d.txt", fileCount);
   ++fileCount;

   imageFile = fopen(imageFileName, "w+");
   positionFile = fopen(positionFileName, "w+");

   //Get the image and write the data to the image file created.
   data = (char *)sendRequest("GET IMAGE", &length);
   while(length > 0) {
      fprintf(imageFile, "%c", *data++);
      --length;
   }
   
   //The imageFile is no longer needed.
   fclose(imageFile);

   //Get GPS data from robot and print to positionFile
   data = (char *)sendRequest("GET GPS", &length);
   fprintf(positionFile, "GPS ");
   while(length > 0) {
      fprintf(positionFile, "%c", *data++);
      --length;
   }
   fprintf(positionFile, "\n");

   //Get DGPS data from robot and print to positionFile
   data = (char *)sendRequest("GET DGPS", &length);
   fprintf(positionFile, "DGPS ");
   while(length > 0) {
      fprintf(positionFile, "%c", *data++);
      --length;
   }
   fprintf(positionFile, "\n");

   //Get LASER data from robot and print to positionFile
   data = (char *)sendRequest("GET LASERS", &length);
   fprintf(positionFile, "LASERS ");
   while(length > 0) {
      fprintf(positionFile, "%c", *data++);
      --length;
   }
   fprintf(positionFile, "\n");

   fclose(positionFile);

   return;

}



int main(int argc, char** argv) {
	//get command line args
	if(argc != 5) {
		fprintf(stderr, "Usage: %s <server IP or server host name> <server port> <L> <N>\n", argv[0]);
		exit(1);	
	}
	char* serverHost = argv[1];
	char* serverPort = argv[2];
	if(!sscanf(argv[3], "%lf", &L) || L <= 0) {
		fprintf(stderr, "L must be greater than 0");
		exit(1);
	}
	N = atoi(argv[4]);
	if(N < 4 || N > 8) {
		fprintf(stderr, "N must be an integer the range [4, 8]");
		exit(1);
	}
	
	setupMessenger(serverHost, serverPort);
	//clientMessenger test code
	int length;
	char* response = sendRequest("", &length);
	int i;
	for(i = 0; i < length; i++) {
		printf("%c", response[i]);
	}
	printf("\n");
	
	//tracePolygon(N, true);
	//tracePolygon(N-1, false);

	return 0;
}
