CFLAG= -std=c++11 -I ../../build/release-install-cpp11/include/
LFLAG= -L ../../build/release-install-cpp11/lib/ -lmuduo_net -lmuduo_base -lpthread -ljsoncpp
all:server client register

server:s_test1.cpp
	g++ $(CFLAG) -g -o $@ $^  $(LFLAG)
client:c_test1.cpp
	g++ $(CFLAG) -g -o $@ $^  $(LFLAG)
register:register_server.cpp
	g++ $(CFLAG) -g -o $@ $^  $(LFLAG)
.PHONY:clean
clean:
	rm -rf server client register