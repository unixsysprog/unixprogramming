CC = gcc
CFLAGS = -g -Wall

default: pong


pong: pongimp.o wall.o ball.o alarmlib.o paddle.o draw.o
	$(CC) $(CFLAGS) pongimp.o wall.o ball.o alarmlib.o \
	 paddle.o draw.o -o pong -lrt -lcurses


pongimp.o: pong.c wall.h alarmlib.h paddle.h ball.h draw.h
	$(CC) $(CFLAGS) -c pong.c -o pongimp.o

wall.o: wall.c wall.h draw.h
	$(CC) $(CFLAGS) -c wall.c -o wall.o

paddle.o: paddle.c paddle.h wall.h draw.h
	$(CC) $(CFLAGS) -c paddle.c -o paddle.o

ball.o: ball.c ball.h wall.h paddle.h draw.h
	$(CC) $(CFLAGS) -c ball.c -o ball.o

draw.o: draw.c draw.h
	$(CC) $(CFLAGS) -c draw.c -o draw.o

alarmlib.o: alarmlib.c alarmlib.h
	$(CC) $(CFLAGS) -c alarmlib.c

clean: 
	$(RM) -f pong *.o
