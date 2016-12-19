CC      = /usr/bin/gcc
CFLAGS  = -I./inc -Wall -O3 `pkg-config --cflags glib-2.0 libconfig`
LDFLAGS = -I./inc `pkg-config --libs glib-2.0 gio-2.0 libconfig`
DEPENDFILE = .depend

SRC = nuimod.c nuimo_communication.c

OBJ = nuimod.o nuimo_communication.o inc/nuimo.o

BIN = nuimod

all:	nuimo_inc nuimod

debug:	CFLAGS += -DDEBUG -g
debug:	nuimod

nuimod:	$(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS)

nuimo_inc:
	cd inc && make nuimo.o

%.o:	%.c
	$(CC) $(CFLAGS) -c $<


clean:
	cd inc && make clean
	rm -rf $(BIN) $(OBJ)

.PHONY:	clean all
