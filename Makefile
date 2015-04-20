CC = gcc
CFLAGS = -Wall
PROGS =	robotClient robotServer dummyServer

all: $(PROGS)

robotClient: client.c clientMessenger.h clientMessenger.c Makefile
	${CC} -o $@ client.c clientMessenger.c ${CFLAGS}

robotServer: server.c serverMessenger.h serverMessenger.c Makefile
	${CC} -o $@ server.c serverMessenger.c ${CFLAGS}

dummyServer: dummyServer.c serverMessenger.h serverMessenger.c Makefile
	${CC} -o $@ dummyServer.c serverMessenger.c ${CFLAGS}

clean:
	rm -f ${PROGS}