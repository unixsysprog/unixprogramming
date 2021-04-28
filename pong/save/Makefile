CC = gcc
CFLAGS = -g -Wall

default: pong


pong: pong.o wall.o ball.o alarmlib.o paddle.o
	$(CC) $(CFLAGS) pong.o wall.o ball.o alarmlib.o paddle.o -o pong -lrt -lcurses


pong.o: pong.c wall.h alarmlib.h paddle.h ball.h
	$(CC) $(CFLAGS) -c pong.c -o pong.o -lrt -lcurses

wall.o: wall.c wall.h
	$(CC) $(CFLAGS) -c wall.c -o wall.o -lcurses

paddle.o: paddle.c paddle.h wall.h
	$(CC) $(CFLAGS) -c paddle.c -o paddle.o -lcurses

ball.o: ball.c ball.h wall.h paddle.h
	$(CC) $(CFLAGS) -c ball.c -o ball.o -lcurses

alarmlib.o: alarmlib.c alarmlib.h
	$(CC) $(CFLAGS) -c alarmlib.c

clean: 
	$(RM) -f pong *.o
