CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2
LDFLAGS = -lcurl -ljson-c
CACHE_TARGET = git-cache
GITHUB_TARGET = github_test
URL_TEST_TARGET = test_url_parsing
FORK_TEST_TARGET = test_fork_integration
SUBMODULE_TEST_TARGET = test_submodule
METADATA_TEST_TARGET = test_cache_metadata
CACHE_SOURCES = git-cache.c github_api.c submodule.c cache_recovery.c cache_metadata.c checkout_repair.c strategy_detection.c
GITHUB_SOURCES = github_api.c
CACHE_OBJECTS = $(CACHE_SOURCES:.c=.o)
GITHUB_OBJECTS = $(GITHUB_SOURCES:.c=.o)
HEADERS = git-cache.h github_api.h submodule.h cache_recovery.h cache_metadata.h checkout_repair.h strategy_detection.h

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

.PHONY: all clean install uninstall github-test cache-test url-test-run fork-test-run robustness-test concurrent-test test-all clean-cache clean-all help

all: $(CACHE_TARGET)

cache: $(CACHE_TARGET)

github: $(GITHUB_TARGET)

url-test: $(URL_TEST_TARGET)

fork-test: $(FORK_TEST_TARGET)

submodule-test: $(SUBMODULE_TEST_TARGET)

metadata-test: $(METADATA_TEST_TARGET)

$(CACHE_TARGET): $(CACHE_OBJECTS)
	$(CC) $(CACHE_OBJECTS) -o $@ $(LDFLAGS)

$(GITHUB_TARGET): $(GITHUB_OBJECTS) github_test.o
	$(CC) $(GITHUB_OBJECTS) github_test.o -o $@ $(LDFLAGS)

$(URL_TEST_TARGET): github_api.o test_url_parsing.o
	$(CC) github_api.o test_url_parsing.o -o $@ $(LDFLAGS)

$(FORK_TEST_TARGET): test_fork_integration.o
	$(CC) test_fork_integration.o -o $@

$(SUBMODULE_TEST_TARGET): test_submodule.o submodule.o repo_info_stub.o
	$(CC) test_submodule.o submodule.o repo_info_stub.o -o $@

$(METADATA_TEST_TARGET): test_cache_metadata.o cache_metadata.o metadata_test_stub.o
	$(CC) test_cache_metadata.o cache_metadata.o metadata_test_stub.o -o $@ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CACHE_OBJECTS) $(GITHUB_OBJECTS) github_test.o test_url_parsing.o test_fork_integration.o test_submodule.o submodule.o test_cache_metadata.o metadata_test_stub.o $(CACHE_TARGET) $(GITHUB_TARGET) $(URL_TEST_TARGET) $(FORK_TEST_TARGET) $(SUBMODULE_TEST_TARGET) $(METADATA_TEST_TARGET)

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
	@echo "  all          Build git-cache program"
	@echo "  cache        Build git-cache program only"
	@echo "  github       Build github_test program only"
	@echo "  url-test     Build URL parsing test program"
	@echo ""
	@echo "Test targets:"
	@echo "  github-test     Run GitHub API tests"
	@echo "  cache-test      Run git-cache integration tests"
	@echo "  url-test-run    Run URL parsing tests"
	@echo "  robustness-test Run robustness and failure recovery tests"
	@echo "  concurrent-test Run concurrent execution tests"
	@echo "  test-all        Run all test suites"
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

install: $(CACHE_TARGET)
	install -d $(BINDIR)
	install -m 755 $(CACHE_TARGET) $(BINDIR)/

uninstall:
	rm -f $(BINDIR)/$(CACHE_TARGET)

# Legacy test target removed - use specific test targets instead

github-test: $(GITHUB_TARGET)
	./tests/run_github_tests.sh

cache-test: $(CACHE_TARGET)
	./tests/run_cache_tests.sh

url-test-run: $(URL_TEST_TARGET)
	./tests/run_url_tests.sh

fork-test-run: $(FORK_TEST_TARGET)
	./$(FORK_TEST_TARGET)

robustness-test: $(CACHE_TARGET)
	./tests/run_robustness_tests.sh

concurrent-test: $(CACHE_TARGET)
	./test_concurrent.sh

test-all: $(CACHE_TARGET) $(URL_TEST_TARGET)
	./tests/run_all_tests.sh

debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

static: CFLAGS += -static
static: $(TARGET)

.SUFFIXES: .c .o