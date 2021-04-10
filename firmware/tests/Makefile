CFLAGS = -O1 -g
LDLIBS =
OBJS = testmain.o ../termcore.o

all: test

clean:
	rm test ${OBJS}

test: ${OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${OBJS} ${LDLIBS}
