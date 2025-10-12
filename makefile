CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = wish

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: $(TARGET) run


$(TARGET): wish.o parallel.o program_array.o utils.o command.o
	$(CC) $(CFLAGS) -o $@ wish.o parallel.o program_array.o utils.o command.o

parallel_test: parallel_test.o parallel.o
	$(CC) $(CFLAGS) -o $@ parallel_test.o parallel.o

run: $(TARGET)
	./$(TARGET) $(ARGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJ) $(TARGET) parallel_test

.PHONY: all clean run parallel_test
