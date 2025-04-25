# Compilador e flags
CC = gcc
CFLAGS = -w -O2 -g -Iinclude `pkg-config --cflags glib-2.0`
LDFLAGS = -w -O2 -g
LIBS = `pkg-config --libs glib-2.0`

# Ficheiros
SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c, obj/%.o, $(SRCS))

# Targets finais
TARGETS := bin/dserver bin/dclient

# Target principal
all: folders $(TARGETS)

# Criação das pastas
folders:
	@mkdir -p obj bin

# Como compilar os binários
bin/dserver: obj/dserver.o obj/utils.o obj/executar.o obj/cache.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/dclient: obj/dclient.o obj/utils.o obj/executar.o obj/cache.o
	$(CC) $(LDFLAGS) $^ -o $@

# Compilar os .o a partir dos .c
obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf obj/* bin/*

.PHONY: all clean folders
