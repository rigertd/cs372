# Author:  David Rigert
# Created: 4/30/2016
# CS372 Project 1: chatserve makefile

CXX = g++
CXXFLAGS = -std=c++11 -O3 -pthread -Wl,--no-as-needed
SOURCE = chatserve.cpp Socket.cpp SocketStream.cpp

all: $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o chatserve

clean:
	$(RM) -f chatserve
