#include "clientMessenger.h"
#include "quit.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

void tracePolygon(int numSides, bool clockwise);
void getSnapshot();

double L; //L >= 1
int N; //4 <= N <= 8
int fileCount = 0;

const int COMMAND_TIMEOUT = 1;
const int DATA_TIMEOUT = 5;

void tracePolygon(int numSides, bool clockwise) {
   int dummy;
   
   //Determine the angle the robot should turn.
   double turnAngle = 180.0 - ((numSides - 2)*180.0/numSides);
   turnAngle = turnAngle*2*M_PI/360;
   if(!clockwise) turnAngle = turnAngle*-1;
   
   //Create a turn request for pi/4 radians per second.
   char *turnRequest = (char *)malloc(20);
   sprintf(turnRequest, "TURN %.10f", M_PI/4);
   
   //Take initial screenshot before making the polygon.
   getSnapshot();

   //Logic for tracing the polygon.
   int i;
   for(i = 0; i < numSides; i++) {
      sendRequest("MOVE 1", &dummy, COMMAND_TIMEOUT);
      //Wait for L seconds.
      sendRequest("STOP", &dummy, COMMAND_TIMEOUT);

      getSnapshot();

      sendRequest(turnRequest, &dummy, COMMAND_TIMEOUT);
      //Wait for turnAngle/(M_PI/4)
      sendRequest("STOP", &dummy, COMMAND_TIMEOUT);
   }

   return;
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
   data = (char *)sendRequest("GET IMAGE", &length, DATA_TIMEOUT);
   //while(length > 0) {
   //   fprintf(imageFile, "%c", *data++);
   //   --length;
   //}
   if(fwrite(data, 1, length, imageFile) != length) quit("fwrite failed");
   
   //The imageFile is no longer needed.
   fclose(imageFile);

   //Get GPS data from robot and print to positionFile
   data = (char *)sendRequest("GET GPS", &length, DATA_TIMEOUT);
   fprintf(positionFile, "GPS ");
   //while(length > 0) {
   //   fprintf(positionFile, "%c", *data++);
   //   --length;
   //}
   if(fwrite(data, 1, length, positionFile) != length) quit("fwrite failed");

   fprintf(positionFile, "\n");

   //Get DGPS data from robot and print to positionFile
   data = (char *)sendRequest("GET DGPS", &length, DATA_TIMEOUT);
   fprintf(positionFile, "DGPS ");
   //while(length > 0) {
   //   fprintf(positionFile, "%c", *data++);
   //   --length;
   //}
   if(fwrite(data, 1, length, positionFile) != length) quit("fwrite failed");

   fprintf(positionFile, "\n");

   //Get LASER data from robot and print to positionFile
   data = (char *)sendRequest("GET LASERS", &length, DATA_TIMEOUT);
   fprintf(positionFile, "LASERS ");
   //while(length > 0) {
   //   fprintf(positionFile, "%c", *data++);
   //   --length;
   //}
   if(fwrite(data, 1, length, positionFile) != length) quit("fwrite failed");
   
   fprintf(positionFile, "\n");

   fclose(positionFile);

   return;
}

int main(int argc, char** argv) {
	//get command line args
	if(argc != 6) {
		fprintf(stderr, "Usage: %s <server IP or server host name> <server port> <ID> <L> <N>\n", argv[0]);
		exit(1);	
	}
	char* serverHost = argv[1];
	char* serverPort = argv[2];
        char* robotID = argv[3];
	if(!sscanf(argv[4], "%lf", &L) || L <= 1) {
		fprintf(stderr, "L must be at least 1");
		exit(1);
	}
	N = atoi(argv[5]);
	if(N < 4 || N > 8) {
		fprintf(stderr, "N must be an integer the range [4, 8]");
		exit(1);
	}
	
	setupMessenger(serverHost, serverPort, robotID);
	
	tracePolygon(N, true);
	tracePolygon(N-1, false);

	return 0;
}
