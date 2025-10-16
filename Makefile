.PHONY: all build clean deps install run help

all: build

deps:
	@echo "Building dependencies..."
	./build_deps.sh

build: deps
	@echo "Building img-boost..."
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$$(nproc)

build-debug: deps
	@echo "Building img-boost (Debug)..."
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$$(nproc)

build-static: deps
	@echo "Building img-boost (Static)..."
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC=ON .. && make -j$$(nproc)

clean:
	@echo "Cleaning build files..."
	rm -rf build

clean-all: clean
	@echo "Cleaning dependencies..."
	rm -rf build_deps third_party

install: build
	@echo "Installing img-boost..."
	cd build && sudo make install

run: build
	@echo "Running img-boost..."
	./build/img-boost

help:
	@echo "Available targets:"
	@echo "  make build        - Build the project (default)"
	@echo "  make build-debug  - Build with debug symbols"
	@echo "  make build-static - Build with static libraries"
	@echo "  make deps         - Build dependencies only"
	@echo "  make clean        - Clean build files"
	@echo "  make clean-all    - Clean build files and dependencies"
	@echo "  make install      - Install to system"
	@echo "  make run          - Build and run the program"
	@echo "  make help         - Show this help message"
