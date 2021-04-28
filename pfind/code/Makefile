# This is a makefile.  A makefile contains rules that tell
# how to build a program, often from separate source files
#

CC = gcc
CFLAGS = -g -Wall

pfind: pfind.o
	$(CC) $(CFLAGS) pfind.o -o pfind

pfind.o: pfind.c
	$(CC) $(CFLAGS) -c pfind.c

clean:
	rm -f pfind *.o
