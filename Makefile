CC		= gcc
PLATFORM= __APPLE__
CFLAGS	= -std=c99 -c -g -Wall -pedantic -D$(PLATFORM) 
LFLAGS	= -lm -framework GLUT -framework OpenGL
LIBS	=
OBJS	= wopengl.o wicom.o debug.o jmlist.o

#.SUFFIXES: .o .c
#.c.o:
#	$(CC) $(CFLAGS) -c $<

wicom: $(OBJS)
	$(CC) $(LFLAGS) -o wicom $(OBJS) $(LIBS)

wicom.o: wicom.c
	$(CC) $(CFLAGS) -o wicom.o wicom.c

wopengl.o: wopengl.c wopengl.h
	$(CC) $(CFLAGS) -o wopengl.o wopengl.c

debug.o: debug.c debug.h
	$(CC) $(CFLAGS) -o debug.o debug.c

jmlist.o: jmlist.c jmlist.h
	$(CC) $(CFLAGS) -o jmlist.o jmlist.c

#%.o: %.c
#	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm *.o
