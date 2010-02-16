CC		= gcc
CFLAGS	= -std=c99 -c -g -Wall -pedantic -I/opt/local/include/ -I/usr/X11/include
LFLAGS	= -lm -framework GLUT -framework OpenGL -lpthread
LIBS	=
OBJS	= wview_fglut.o wicom.o debug.o jmlist.o wlock.o wthread.o

#.SUFFIXES: .o .c
#.c.o:
#	$(CC) $(CFLAGS) -c $<

wicom: $(OBJS)
	$(CC) $(LFLAGS) -o wicom $(OBJS) $(LIBS)

wicom.o: wicom.c
	$(CC) $(CFLAGS) -o wicom.o wicom.c

debug.o: debug.c debug.h
	$(CC) $(CFLAGS) -o debug.o debug.c

jmlist.o: jmlist.c jmlist.h
	$(CC) $(CFLAGS) -o jmlist.o jmlist.c

wlock.o: wlock.c wlock.h
	$(CC) $(CFLAGS) -o wlock.o wlock.c

wthread.o: wthread.c wthread.h
	$(CC) $(CFLAGS) -o wthread.o wthread.c

wview_fglut.o: wview_fglut.c wview.h
	$(CC) $(CFLAGS) -o wview_fglut.o wview_fglut.c

#%.o: %.c
#	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm *.o
