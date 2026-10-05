.PHONY: build test exec clear deps

build:
	cmake -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++
	cmake --build build

test: build
	./build/tests/lexer_tests

exec:
	./build/lexer_fm

clear:
	rm -rf build

deps:
	bash install_deps.sh