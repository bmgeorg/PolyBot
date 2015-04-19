#include "clientMessenger.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

void tracePolygon(int numSides, bool clockwise);
void getSnapshot();

double L; //L >= 1
int N; //4 <= N <= 8
int fileCount = 0;

void tracePolygon(int numSides, bool clockwise) {
   int dummy;
   
   //Determine the angle the robot should turn.
   double turnAngle = 180.0 - ((numSides - 2)*180.0/numSides);
   turnAngle = turnAngle*2*M_PI/360;
   
   //Create a turn request for pi/4 radians per second.
   char *turnRequest = (char *)malloc(50);
   sprintf(turnRequest, "TURN %.10f", M_PI/4);
   
   //Take initial screenshot before making the polygon.
   getSnapshot();

   //Logic for tracing the polygon.
   int i;
   for(i = 0; i < numSides; i++) {
      sendRequest("MOVE 1", &dummy);
      //Wait for L seconds.
      sendRequest("MOVE 0", &dummy);

      getSnapshot();

      sendRequest(turnRequest, &dummy);
      //Wait for turnAngle/(M_PI/4)
      sendRequest("TURN 0", &dummy);
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
	if(!sscanf(argv[3], "%lf", &L) || L <= 1) {
		fprintf(stderr, "L must be at least 1");
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
