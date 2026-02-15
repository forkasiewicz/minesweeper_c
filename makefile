CC = clang

TARGET = game

BUILD_DIR = build

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

CFLAGS = -Wall -Wextra -std=c11 -O2 $(shell pkg-config --cflags raylib)
LDFLAGS = $(shell pkg-config --libs raylib)

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(BUILD_DIR)/$(TARGET)

debug: CFLAGS += -g -O0 -fsanitize=address,undefined
debug: clean all

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run debug
