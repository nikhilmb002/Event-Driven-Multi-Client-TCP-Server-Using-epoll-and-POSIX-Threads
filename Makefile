CC=gcc
CFLAGS=-Wall -Wextra
TARGET=server
SRC=src/main.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
