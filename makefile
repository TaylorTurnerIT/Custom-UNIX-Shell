CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = wish

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: $(TARGET) run

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run:
	./$(TARGET) $(ARGS)

.PHONY: all clean run