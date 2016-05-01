/*********************************************************\
* Author:       David Rigert
* Class:        CS372 Spring 2016
* Assignment:   Project 1
* File:         SocketStream.hpp
* Description:  Defines the class used for interacting with
*               a socket after a connection is established.
\*********************************************************/
#pragma once

#include <string>

class SocketStream {
    public:
        SocketStream(int, std::string, std::string);

        bool send(std::string data);
        bool recv(std::string& buffer);
        void close();

        std::string get_hostname() { return _hostname; }
        std::string get_port() { return _port; }

        private:
        int _sd;                // Underlying socket descriptor
        std::string _hostname;  // Name of connected client
        std::string _port;      // Port number of connected client
};
