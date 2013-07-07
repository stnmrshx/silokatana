CC = gcc
DEBUG =	-g -ggdb -DDEBUG
CFLAGS =  -c -std=c99 -W -Wall -Werror -fPIC  $(DEBUG)

LIB_OBJS = \
	./storage/src/ht.o\
	./storage/src/db.o\
	./storage/src/silopit.o\
	./storage/src/util.o\
	./storage/src/meta.o\
	./storage/src/shiki.o\
	./storage/src/debug.o\
	./storage/src/level.o\
	./storage/src/index.o\
	./storage/src/hiraishin.o\
	./storage/src/buffer.o\
	./storage/src/jikukan.o\
	./storage/src/log.o

LIBRARY = libsilokatana.so

all: $(LIBRARY)

clean:
	-rm -f $(LIBRARY)  
	-rm -f $(LIB_OBJS)
	-rm -f bench/silo-benchmark.o
	-rm -f silo-benchmark

cleandb:
	-rm -rf silodbs
	-rm -rf *.event

$(LIBRARY): $(LIB_OBJS)
	$(CC) -pthread  -fPIC -shared $(LIB_OBJS) -o libsilokatana.so

silo-benchmark: bench/silo-benchmark.o $(LIB_OBJS)
	$(CC) -pthread  bench/silo-benchmark.o $(LIB_OBJS) -o $@