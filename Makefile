CC=gcc
CFLAGS=-Wall -Wextra -pthread
TARGET=server
SRC=src/main.c src/thread_pool.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
