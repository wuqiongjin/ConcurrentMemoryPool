LD_FLAGS=-std=c++11 -lpthread #-g

.PHONY:ALL 
ALL:main

main:BenchMark.o ThreadCache.o CentralCache.o PageCache.o
	g++ -o $@ $^ $(LD_FLAGS)

BenchMark.o:BenchMark.cpp
	g++ -c $^ -o $@ $(LD_FLAGS)
ThreadCache.o:ThreadCache.cpp
	g++ -c $^ -o $@ $(LD_FLAGS)
CentralCache.o:CentralCache.cpp
	g++ -c $^ -o $@ $(LD_FLAGS)
PageCache.o:PageCache.cpp
	g++ -c $^ -o $@ $(LD_FLAGS)

.PHONY:clean
clean:
	rm -rf main *.o
