/*********************************************************\
* Author:       David Rigert
* Class:        CS372 Spring 2016
* Assignment:   Project 1
* File:         Socket.hpp
* Description:  Defines the class used for interacting with
*               a socket before a connection is established.
*               This class is intended for use by a server and
*               only accepts incoming connections.
\*********************************************************/
#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h> /* SOCK_STREAM */

// Define a maximum connection queue of 10 unless defined elsewhere
#ifndef SOCKET_CONNECTION_QUEUE
#define SOCKET_CONNECTION_QUEUE 10
#endif

// Forward declaration
class SocketStream;

class Socket {
    public:
        Socket(int queuelen = SOCKET_CONNECTION_QUEUE);
        ~Socket();

        void listen(const char* port);
        SocketStream accept();

    private:
        int _sd;                // Underlying socket descriptor
        int _queue_len;         // Max incoming connections to queue
        struct addrinfo* _info; // Used for address info lookup

        void get_remote_addr(struct sockaddr*, std::string&, std::string&);
};
