#ifndef SERVER_MESSENGER_H
#define SERVER_MESSENGER_H

#include <netdb.h>	//for struct sockaddr*

/*
	serverMessenger handles all communication with robotClient. 
*/

extern const int RESPONSE_MESSAGE_SIZE;

//public functions
//Send response, chunking into RESPONSE_MESSAGE_SIZE-sized chunks
void sendResponse(struct sockaddr* recipient, int ID, void* response, int responseLength);

#endif