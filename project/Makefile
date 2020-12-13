CFLAGS = -o
CC = gcc
LIBS =  -lm

all: client server

client: client.c
	${CC} ${CFLAGS} client client.c

server: server.c
	${CC} ${CFLAGS} server server.c && ./server 3000

clean:
	rm -f *.o *~

.PHONY: all clean

