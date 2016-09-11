#Do not use this Makefile to build the android version. 
#Use Android.mk instead by launching ndk-build in the application root path.
CFLAGS=-Wall -o
CC=gcc

all:
	mkdir -p ../bin/linux
	$(CC) $(CFLAGS) ../bin/linux/tedproxy utils.c network.c tederror.c main.c
clean: 
	@rm -rfv *.o *.so *.so*

