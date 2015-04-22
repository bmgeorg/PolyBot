CC = gcc
CFLAGS = -Wall
PROGS =	robotClient robotServer dummyServer
DEPS = quit.h quit.c Makefile

all: $(PROGS)

robotClient: client.c clientMessenger.h clientMessenger.c $(DEPS)
	${CC} -o $@ client.c clientMessenger.c quit.c ${CFLAGS}

robotServer: server.c serverMessenger.h serverMessenger.c $(DEPS)
	${CC} -o $@ server.c serverMessenger.c quit.c ${CFLAGS}

dummyServer: dummyServer.c serverMessenger.h serverMessenger.c $(DEPS)
	${CC} -o $@ dummyServer.c serverMessenger.c quit.c ${CFLAGS}

clean:
	rm -f ${PROGS}