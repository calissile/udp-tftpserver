#Sample Makefile. You can make changes to this file according to your need
# The executable must be named proxy

CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -lpthread

OBJS = tftp2.o csapp.o

all: tftp2

tftp2: $(OBJS)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

tftp2.o: tftp2.c
	$(CC) $(CFLAGS) -c tftp2.c
clean:
	rm -f *~ *.o tftp2

