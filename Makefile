CC = gcc-12
CFLAGS = -Iinclude -Wall -Wextra -std=gnu11
SRCS = main.c $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
