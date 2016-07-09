#Do not use this Makefile to build the android version. 
#Use Android.mk instead by launching ndk-build in the application root path.
CFLAGS=-Wall -o
CC=gcc

all:
	$(CC) $(CFLAGS) sendclientapp utils.c network.c tederror.c sendclientapp.c `pkg-config --cflags`  
clean: 
	@rm -rfv *.o *.so *.so*

