# Author:  David Rigert
# Created: 5/22/2016
# CS372 Project 2: ftserve makefile

CXX = g++
CXXFLAGS = -std=c++11
SOURCE = ftserve.cpp

all: ftserve ftclient
	chmod +x ftclient

ftserve: $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o ftserve

clean:
	$(RM) ftserve
	chmod -x ftclient
