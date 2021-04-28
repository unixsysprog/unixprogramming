# This is a Makefile for sttyl

CC = gcc
CFLAGS = -g -Wall

default: sttyl

sttyl: sttyl.o flags.o
	$(CC) $(CFLAGS) sttyl.o flags.o -o sttyl

sttyl.o: sttyl.c flags.h
	$(CC) $(CFLAGS) -c sttyl.c

flags.o: flags.c flags.h
	$(CC) $(CFLAGS) -c flags.c

clean: 
	$(RM) -f sttyl *.o
