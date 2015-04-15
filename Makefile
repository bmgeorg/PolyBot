CC = gcc
CFLAGS = -Wall
PROGS =	robotClient dummyServer

all: $(PROGS)

robotClient: client.c clientMessenger.h clientMessenger.c Makefile
	${CC} -o $@ client.c clientMessenger.c ${CFLAGS}
	
dummyServer: dummyServer.c Makefile
	${CC} -o $@ dummyServer.c ${CFLAGS}

clean:
	rm -f ${PROGS}