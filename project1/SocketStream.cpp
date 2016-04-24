#include "SocketStream.hpp"

#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h> // recv, send
#include <unistd.h>     // close
#include <exception>

#define BUFFER_SIZE 500

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

bool SocketStream::recv(std::string& buffer) {
    ssize_t bytes;
    char buf[BUFFER_SIZE];
    buffer.clear();

    /* Keep trying until all data is received */
    while (true) {
        bytes = ::recv(_sd, buf, BUFFER_SIZE, 0);
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
        buffer += buf;
    }

    // Return true if socket is still open
    return true;
}