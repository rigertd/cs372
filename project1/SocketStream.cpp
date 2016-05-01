/*********************************************************\
* Author:       David Rigert
* Class:        CS372 Spring 2016
* Assignment:   Project 1
* File:         SocketStream.cpp
* Description:  Implementation file for SocketStream.hpp
\*********************************************************/
#include "SocketStream.hpp"

#include <iostream>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h> // recv, send
#include <unistd.h>     // close
#include <exception>
#include <stdexcept>

// Define buffer size of 500 + 1 for null terminator
#define BUFFER_SIZE 501

/**
 * Constructor. Sets the underlying socket descriptor and it to non-blocking.
 *
 *  sock_desc   The socket descriptor to use.
 *  hostname    The hostname of the connected client.
 *  port        The port number of the connected client.
 */
SocketStream::SocketStream(int sock_desc, std::string hostname, std::string port) {
    _sd = sock_desc;
    _hostname = hostname;
    _port = port;

    // Set socket to be non-blocking for receives
    fcntl(_sd, F_SETFL, O_NONBLOCK);
}

/**
 * Sends the specified data to the connected host.
 *
 * This function continues to send until all data has been sent
 * or the socket is closed.
 *
 *  data    The data to send over the socket.
 *
 * Returns whether the socket is still open.
 */
bool SocketStream::send(std::string data) {
    ssize_t bytes;

    // Send until there is nothing left to send
    while (!data.empty()) {
        bytes = ::send(_sd, data.c_str(), data.size(), 0);
        if (bytes == 0) {
            // Socket was closed, return false
            return false;
        } else if (bytes == -1) {
            if (errno == EINTR) {
                // Keep sending if interrupted by signal
                continue;
            }
            else {
                // Some non-interrupt error occurred. Throw exception.
                std::string errmsg("send: ");
                errmsg += ::strerror(errno);
                throw std::runtime_error(errmsg);
            }
        }

        // Remove the sent characters
        data = data.substr(bytes);
    }

    // Return true if socket is still open
    return true;
}

/**
 * Receives any data available from the connected host.
 *
 * This function receives whatever data is available without blocking.
 * If no data is available, buffer is set to an empty string
 * and the function returns true.
 *
 *  buffer  A string buffer to store the received data.
 *
 * Returns whether the socket is still open.
 */
bool SocketStream::recv(std::string& buffer) {
    ssize_t bytes;
    char buf[BUFFER_SIZE];
    buffer.clear();

    /* Keep trying until all data is received */
    while (true) {
        bytes = ::recv(_sd, buf, BUFFER_SIZE - 1, 0);
        // Socket was closed, return false
        if (bytes == 0) {
            return false;
        } else if (bytes == -1) {
            if (errno == EINTR) {
                // Try again if interrupted by signal
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more data to receive
                break;
            }
            /* otherwise, throw exception */
            else {
                std::string errmsg("recv: ");
                errmsg += ::strerror(errno);
                throw std::runtime_error(errmsg);
            }
        }
        buf[bytes] = '\0';
        buffer += buf;
    }

    // Return true if socket is still open
    return true;
}

/**
 * Closes the socket.
 *
 * This function immediately closes the underlying socket descriptor.
 * The object can no longer be used for sending or receiving after
 * this function is called.
 */
void SocketStream::close() {
    ::close(_sd);
}
