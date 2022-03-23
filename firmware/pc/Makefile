CFLAGS = -O1 -g $(shell pkg-config sdl --cflags) -I.
LDLIBS = $(shell pkg-config sdl --libs)
OBJS = pcmain.o pico_stdlib.o serial.o ../graphics.o ../terminal.o ../termcore.o

all: pcmain

clean:
	rm -f pcmain ${OBJS}

pcmain: ${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${OBJS} ${LDLIBS}
