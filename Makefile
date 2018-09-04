OBJ = xephyr-helper.o functions.o xclib.o
DEPS = functions.h xclib.h stdafx.h
LIBS = -lX11 -lXi -lXmu
CFLAGS = -Wall -Os -s
CC = gcc
EXTENSION = .c

%.o: %$(EXTENSION) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

xephyr-helper: $(OBJ_XEP)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o *~ core *~ main
