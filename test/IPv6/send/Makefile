CFLAGS=-Wall -o
CC=gcc

all:	libsend.c sendrecvUDP.c testlib.c sendclientapp.c
	$(CC) $(CFLAGS) sendclientapp utils.c libsend.c sendrecvUDP.c testlib.c sendclientapp.c -ljson-c
	

clean: 
	@rm -rfv *.o *.so *.so*

