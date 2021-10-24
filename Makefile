CC=clang++
CXX = clang++
CXXFLAGS = -std=c++14 -stdlib=libc++ -c -g -O0 -Wall -Wextra -Werror -pedantic
DEPS=BrokerClient.hpp
OBJ=BrokerClient.o BrokerClientTests.o

test: $(OBJ)
	$(CC) -o $@ $^ -std=c++11

.PHONY: clean

clean:
	rm -rf *.o test
