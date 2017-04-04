disk: disk.cc libthread.o
	g++ disk.cc libthread.o -g -pg -ldl -pthread -std=c++11 -o disk

clean:
	rm disk