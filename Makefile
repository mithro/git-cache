CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2
LDFLAGS = 
TARGET = git-mycommand
SOURCES = git-mycommand.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = git-mycommand.h

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

.PHONY: all clean install uninstall test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/

uninstall:
	rm -f $(BINDIR)/$(TARGET)

test: $(TARGET)
	./tests/run_tests.sh

debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

static: CFLAGS += -static
static: $(TARGET)

.SUFFIXES: .c .o