#include <stdio.h>
#include <stdlib.h>

char* serverHost;
char* serverPort;
uint32_t L;
uint32_t N;

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
	if(atoi(argv[3]) <= 0)
		quit("L must be greater than 0");
	L = atoi(argv[3]);
	if(atoi(argv[4]) <= 0)
		quit("N must be in the range [4, 8]");
	N = atoi(argv[4]);
	
	return true;
}

int main(int argc, char** argv) {
	if(!parseArgs(argc, argv)) {
		fprintf(stderr, "Usage: %s <server IP or host name> <server port> <L> <N>\n", argv[0]);
		exit(1);
	}

	return 0;
}