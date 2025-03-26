all:test

test:test.cc
	g++ -o $@ $^ -std=c++17 -lspdlog -lfmt
clean:
	rm -rf test