#include "clientMessenger.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

double L; //L > 0
int N; //4 <= N <= 8

void tracePolygon(int numSides, bool clockwise) {

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
	
	tracePolygon(N, true);
	tracePolygon(N-1, false);

	return 0;
}