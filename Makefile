all: clean build run
build:
	cd src && g++ -o gg gg.cpp
	mkdir -p build && mv src/gg build/
run: build
		./build/gg
clean:
	rm -rf build/
