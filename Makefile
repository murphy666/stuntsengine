# Root Makefile — thin wrapper around CMake presets.
#
# Targets:
#   all / build    configure + build using $(PRESET)
#   test / check   run the full test suite with colored output
#   clean          remove build artifacts
#   configure      regenerate CMake build files

CMAKE     := cmake
PRESET    ?= linux-debug

.PHONY: all build test check clean configure

all: build

configure:
	$(CMAKE) --preset $(PRESET)

build: configure
	$(CMAKE) --build --preset $(PRESET)

test: build
	$(CMAKE) --build --preset $(PRESET) --target check

check: test

clean:
	$(CMAKE) --build --preset $(PRESET) --target clean
