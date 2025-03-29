CC = gcc
CFLAGS = -w -O2 -funroll-all-loops -ftree-vectorize -Wextra -g `pkg-config --cflags glib-2.0` -Iinclude
LDFLAGS += -lglib-2.0
LIBS = `pkg-config --libs glib-2.0`
all: folders dserver dclient
dserver: bin/dserver
dclient: bin/dclient
folders:
	@mkdir -p src include obj bin tmp
bin/dserver: obj/dserver.o
	$(CC) $(LDFLAGS) $^ -o $@
bin/dclient: obj/dclient.o
	$(CC) $(LDFLAGS) $^ -o $@
obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f obj/* tmp/* bin/*