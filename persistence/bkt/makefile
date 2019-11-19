CC=g++
CXX=g++
CFLAGS = -Wall -Wconversion -O3 -fPIC -fopenmp -g
SHVER = 1
OS = $(shell uname)

all: train

train: utils.o StripedArray.o FitBit.o HMMProblem.o InputUtil.o trainhmm.cpp
	$(CXX) $(CFLAGS) -shared -o trainhmm trainhmm.cpp utils.o FitBit.o InputUtil.o HMMProblem.o StripedArray.o
	#$(CXX) $(CFLAGS) -shared -o trainhmm trainhmm.cpp utils.o FitBit.o InputUtil.o HMMProblem.o StripedArray.o

utils.o: utils.cpp utils.h
	$(CXX) $(CFLAGS) -c -o utils.o utils.cpp

StripedArray.o: StripedArray.cpp StripedArray.h
	$(CXX) $(CFLAGS) -c -o StripedArray.o StripedArray.cpp

InputUtil.o: InputUtil.cpp InputUtil.h
	$(CXX) $(CFLAGS) -c -o InputUtil.o InputUtil.cpp

FitBit.o: FitBit.cpp FitBit.h
	$(CXX) $(CFLAGS) -c -o FitBit.o FitBit.cpp

HMMProblem.o: HMMProblem.cpp HMMProblem.h
	$(CXX) $(CFLAGS) -c -o HMMProblem.o HMMProblem.cpp 

clean:
	rm -f *.o trainhmm

tidy:
	rm -f *.o
