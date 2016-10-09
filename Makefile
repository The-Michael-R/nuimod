CC      = /usr/bin/gcc
CFLAGS  = -I./inc -Wall -O3 `pkg-config --cflags glib-2.0 libconfig`
LDFLAGS = -I./inc `pkg-config --libs glib-2.0 gio-2.0 libconfig`
DEPENDFILE = .depend

SRC = nuimod.c 

OBJ = nuimod.o inc/nuimo.o

BIN = nuimod

all:	nuimo_inc nuimod

debug:	CFLAGS += -DDEBUG -g
debug:	nuimod

nuimod:	$(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS)

nuimo_inc:
	cd inc && make

%.o:	%.c
	$(CC) $(CFLAGS) -c $<


clean:
	rm -rf $(BIN) $(OBJ)

.PHONY:	clean all