CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -Isrc
LDFLAGS ?= -lpthread

TARGET = mini-os

# Fuentes modulares
SRCS = src/main.c \
       src/shell/shell.c \
       src/shell/parser.c \
       src/process/process.c \
       src/process/signals.c \
       src/memory/memory.c \
       src/memory/first_fit.c \
       src/memory/paging.c \
       src/memory/mem_queue.c \
       src/threads/buffer.c \
       src/threads/producer_consumer.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -g3 -O0 -DDEBUG
debug: clean $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJS) $(TARGET) *.dSYM valgrind-out.txt
