build:
	cmake -B build -DCMAKE_CXX_COMPILER=clang++
	cmake --build build

exec:
	./build/lexer_fm

clear:
	rm -rf build

deps:
	bash install_deps.sh