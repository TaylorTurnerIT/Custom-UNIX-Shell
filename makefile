CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = wish

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: $(TARGET) run

all: $(TARGET)


$(TARGET): wish.o parallel.o
	$(CC) $(CFLAGS) -o $@ wish.o parallel.o

parallel_test: parallel_test.o parallel.o
	$(CC) $(CFLAGS) -o $@ parallel_test.o parallel.o

run: $(TARGET)
	./$(TARGET) $(ARGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run:
	./$(TARGET) $(ARGS)

.PHONY: all clean run parallel_test