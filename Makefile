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

SVR_OBJS = \
	./server/src/ae.o\
	./server/src/anet.o\
	./server/src/request.o\
	./server/src/response.o\
	./server/src/zmalloc.o

LIBRARY = libsilokatana.so

all: $(LIBRARY)

clean:
	-rm -f $(LIBRARY)  
	-rm -f silo-benchmark siloserver
	-rm -f bench/silo-benchmark.o server/src/siloserver.o 
	-rm -f silo-benchmark
	-rm -f $(SVR_OBJS)
	-rm -f $(LIB_OBJS)

cleandb:
	-rm -rf silodbs
	-rm -rf *.event

$(LIBRARY): $(LIB_OBJS)
	$(CC) -pthread  -fPIC -shared $(LIB_OBJS) -o libsilokatana.so

silo-benchmark: bench/silo-benchmark.o $(LIB_OBJS)
	$(CC) -pthread  bench/silo-benchmark.o $(LIB_OBJS) -o $@

siloserver: server/src/siloserver.o $(SVR_OBJS) $(LIB_OBJS)
	$(CC) -pthread server/src/siloserver.o $(SVR_OBJS) $(LIB_OBJS) -o $@