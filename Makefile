CC=gcc
CFLAGS=-Wall -Wextra -pthread
TARGET=server
SRC=src/main.c src/thread_pool.c src/shared_memory.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
