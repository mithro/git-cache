CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2
LDFLAGS = -lcurl -ljson-c
TARGET = git-mycommand
CACHE_TARGET = git-cache
GITHUB_TARGET = github_test
URL_TEST_TARGET = test_url_parsing
SOURCES = git-mycommand.c
CACHE_SOURCES = git-cache.c github_api.c
GITHUB_SOURCES = github_api.c
OBJECTS = $(SOURCES:.c=.o)
CACHE_OBJECTS = $(CACHE_SOURCES:.c=.o)
GITHUB_OBJECTS = $(GITHUB_SOURCES:.c=.o)
HEADERS = git-mycommand.h git-cache.h github_api.h

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

.PHONY: all clean install uninstall test github-test cache-test url-test-run test-all clean-cache clean-all help

all: $(TARGET) $(CACHE_TARGET)

cache: $(CACHE_TARGET)

github: $(GITHUB_TARGET)

url-test: $(URL_TEST_TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(CACHE_TARGET): $(CACHE_OBJECTS)
	$(CC) $(CACHE_OBJECTS) -o $@ $(LDFLAGS)

$(GITHUB_TARGET): $(GITHUB_OBJECTS) github_test.o
	$(CC) $(GITHUB_OBJECTS) github_test.o -o $@ $(LDFLAGS)

$(URL_TEST_TARGET): github_api.o test_url_parsing.o
	$(CC) github_api.o test_url_parsing.o -o $@ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(CACHE_OBJECTS) $(GITHUB_OBJECTS) github_test.o test_url_parsing.o $(TARGET) $(CACHE_TARGET) $(GITHUB_TARGET) $(URL_TEST_TARGET)

clean-cache:
	@echo "Cleaning cache and repository directories..."
	rm -rf .cache github
	@echo "Cache cleanup complete"

clean-all: clean clean-cache
	@echo "Full cleanup complete"

help:
	@echo "Git Cache Tool - Makefile Targets"
	@echo "=================================="
	@echo ""
	@echo "Build targets:"
	@echo "  all          Build all programs (git-mycommand and git-cache)"
	@echo "  cache        Build git-cache program only"
	@echo "  github       Build github_test program only"
	@echo ""
	@echo "Test targets:"
	@echo "  test         Run git-mycommand tests"
	@echo "  github-test  Run GitHub API tests"
	@echo "  cache-test   Run git-cache integration tests"
	@echo "  url-test-run Run URL parsing tests"
	@echo "  test-all     Run all test suites"
	@echo ""
	@echo "Cleanup targets:"
	@echo "  clean        Remove compiled objects and binaries"
	@echo "  clean-cache  Remove cache and repository directories (.cache, github)"
	@echo "  clean-all    Full cleanup (clean + clean-cache)"
	@echo ""
	@echo "Install targets:"
	@echo "  install      Install binaries to $(BINDIR)"
	@echo "  uninstall    Remove binaries from $(BINDIR)"
	@echo ""
	@echo "Development targets:"
	@echo "  debug        Build with debug symbols"
	@echo "  static       Build with static linking"
	@echo ""
	@echo "Examples:"
	@echo "  make cache && ./git-cache clone https://github.com/user/repo.git"
	@echo "  make clean-cache  # Clean up test repositories"
	@echo "  make cache-test   # Run full test suite"

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/

uninstall:
	rm -f $(BINDIR)/$(TARGET)

test: $(TARGET)
	./tests/run_tests.sh

github-test: $(GITHUB_TARGET)
	./tests/run_github_tests.sh

cache-test: $(CACHE_TARGET)
	./tests/run_cache_tests.sh

url-test-run: $(URL_TEST_TARGET)
	./tests/run_url_tests.sh

test-all: $(CACHE_TARGET) $(URL_TEST_TARGET)
	./tests/run_all_tests.sh

debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

static: CFLAGS += -static
static: $(TARGET)

.SUFFIXES: .c .o