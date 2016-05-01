# Author:  David Rigert
# Created: 4/30/2016
# CS372 Project 1: chatserve makefile

CXX = g++
CXXFLAGS = -std=c++11 -O3

all: chatserve.cpp Socket.cpp
	$(CXX) $(CXXFLAGS) chatserve.cpp Socket.cpp -o chatserve

clean:
	$(RM) -f chatserve
