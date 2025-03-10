CFLAG= -std=c++11 -I ../../build/release-install-cpp11/include/
LFLAG= -L ../../build/release-install-cpp11/lib/ -lmuduo_net -lmuduo_base -lpthread -ljsoncpp
all : server publish subscribe
server : server.cpp
	g++  -g $(CFLAG) $^ -o $@ $(LFLAG)
publish : publish.cpp
	g++  -g $(CFLAG) $^ -o $@ $(LFLAG)
subscribe: subscribe.cpp
	g++  -g $(CFLAG) $^ -o $@ $(LFLAG)

.PHONY:clean
clean:
	rm -rf server publish subscribe