CC=cc
CFLAGS=-std=c99
basic_server : basic_server.c
		$(CC) -o basic_server basic_server.c
