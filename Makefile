CC = gcc
CFLAGS = -Wall
PROGS =	robotClient

all: $(PROGS)

robotClient: client.c clientMessenger.h clientMessenger.c
	${CC} -o $@ client.c clientMessenger.c ${CFLAGS}

clean:
	rm -f ${PROGS}