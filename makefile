CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
TARGET = wish

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: $(TARGET) run


$(TARGET): wish.o parallel.o program_array.o utils.o command.o timer.o
	$(CC) $(CFLAGS) -o $@ wish.o parallel.o program_array.o utils.o command.o timer.o

parallel_test: parallel_test.o parallel.o
	$(CC) $(CFLAGS) -o $@ parallel_test.o parallel.o

timer_test: timer_test.o timer.o
	$(CC) $(CFLAGS) -o $@ timer_test.o timer.o -pthread

run: $(TARGET)
	./$(TARGET) $(ARGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJ) $(TARGET) parallel_test timer_test timer.o timer_test.o

.PHONY: all clean run parallel_test
