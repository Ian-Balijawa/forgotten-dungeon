# Convenience wrapper around CMake.  Usage: make | make run | make package | make clean
BUILD_DIR  ?= build
BUILD_TYPE ?= Release
JOBS       ?= $(shell sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
UNAME      := $(shell uname -s)

ifeq ($(UNAME),Darwin)
  BIN := $(BUILD_DIR)/ForgottenDungeon.app/Contents/MacOS/ForgottenDungeon
else
  BIN := $(BUILD_DIR)/forgotten-dungeon
endif

.PHONY: all configure build run debug clean package deps

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR) -j$(JOBS)

run: build            # run from the repo root so ./assets is found
	./$(BIN)

debug:
	$(MAKE) BUILD_TYPE=Debug BUILD_DIR=build-debug run

package: build        # .dmg on macOS, .deb/.tar.gz on Linux
	cd $(BUILD_DIR) && cpack -C $(BUILD_TYPE)

clean:
	rm -rf build build-debug

deps:
ifeq ($(UNAME),Darwin)
	brew install cmake
else
	sudo apt-get update && sudo apt-get install -y build-essential cmake git \
	  libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev \
	  libxinerama-dev libwayland-dev libxkbcommon-dev
endif
