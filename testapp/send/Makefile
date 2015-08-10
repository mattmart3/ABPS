CFLAGS=-Wall -o
CC=gcc

all:
	$(CC) $(CFLAGS) sendclientapp utils.c libsend.c tederror.c logger.c sendclientapp.c -ljson-c
	

clean: 
	@rm -rfv *.o *.so *.so*

