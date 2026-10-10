CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude
LDLIBS := -lreadline
SRC := $(wildcard src/*.c)
TARGET := shellforge

.PHONY: all clean run
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDLIBS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) a.out
