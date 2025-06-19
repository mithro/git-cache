CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2
LDFLAGS = -lcurl -ljson-c
TARGET = git-mycommand
GITHUB_TARGET = github_test
SOURCES = git-mycommand.c
GITHUB_SOURCES = github_api.c
OBJECTS = $(SOURCES:.c=.o)
GITHUB_OBJECTS = $(GITHUB_SOURCES:.c=.o)
HEADERS = git-mycommand.h github_api.h

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

.PHONY: all clean install uninstall test github-test

all: $(TARGET)

github: $(GITHUB_TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(GITHUB_TARGET): $(GITHUB_OBJECTS) github_test.o
	$(CC) $(GITHUB_OBJECTS) github_test.o -o $@ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(GITHUB_OBJECTS) github_test.o $(TARGET) $(GITHUB_TARGET)

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/

uninstall:
	rm -f $(BINDIR)/$(TARGET)

test: $(TARGET)
	./tests/run_tests.sh

github-test: $(GITHUB_TARGET)
	./tests/run_github_tests.sh

debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

static: CFLAGS += -static
static: $(TARGET)

.SUFFIXES: .c .o