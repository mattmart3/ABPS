CFLAGS=-Wall -o
CC=gcc

all:
	$(CC) $(CFLAGS) sendclientapp utils.c network.c tederror.c sendclientapp.c `pkg-config --cflags --libs glib-2.0`  
clean: 
	@rm -rfv *.o *.so *.so*

