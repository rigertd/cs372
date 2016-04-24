#include "Socket.hpp"

#include <unistd.h>
#include <cerrno>
#include <netdb.h>
#include <cstring>
#include <exception>
#include <stdexcept>

#include "SocketStream.hpp"

Socket::Socket(int queuelen) {
    _queue_len = queuelen;
    _info = nullptr;
}

Socket::~Socket() {
    ::close(_sd);
    if (_info != nullptr) ::freeaddrinfo(_info);
}

void Socket::listen(const char* port) {
    struct addrinfo hints;
    struct addrinfo *current = nullptr;
    int yes = 1;
    int retval;
    std::string errmsg;
    

    // Zero-initialize and set addrinfo structure
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6, whichever is available
    hints.ai_socktype = SOCK_STREAM; // Non-blocking TCP
    hints.ai_flags = AI_PASSIVE;    // Use localhost IP

    // Look up the localhost address info
    retval = ::getaddrinfo(NULL, port, &hints, &_info);
    if (retval != 0) {
        errmsg = "getaddrinfo: ";
        errmsg += ::gai_strerror(retval);
        throw std::runtime_error(errmsg);
    }

    // Loop through address structure results until bind succeeds
    for (current = _info; current != NULL; current = current->ai_next) {
        // Attempt to open a socket based on the localhost's address info
        _sd = ::socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (_sd == -1) {
            continue; // Try next on error
        }

        // Attempt to reuse the socket if it's already in use
        if (::setsockopt(_sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            errmsg = "setsockopt: ";
            errmsg += ::strerror(errno);
            throw std::runtime_error(errmsg);
        }

        // Attempt to bind the socket to the port
        if (::bind(_sd, current->ai_addr, current->ai_addrlen) == 0) {
            break;  // Break from loop if bind was successful
        }

        // Bind failed. Close file descriptor and try next address
        close(_sd);
    }

    // Throw an exception if bind failed on all returned addresses
    if (current == NULL) {
        errmsg = "bind: No valid address found";
        throw std::runtime_error(errmsg);
    }

    // Free memory used by localhost's address info
    ::freeaddrinfo(_info);
    _info = nullptr;

    // Listen on port for up to _queue_len connections
    if (::listen(_sd, _queue_len) < 0) {
        errmsg = "listen: ";
        errmsg += ::strerror(errno);
        throw std::runtime_error(errmsg);
    }
}

SocketStream Socket::accept() {
    int new_sd;
    struct sockaddr_storage remote_addr;
    socklen_t sin_size = sizeof(remote_addr);
    char s[INET6_ADDRSTRLEN];
    
    new_sd = ::accept(_sd, reinterpret_cast<struct sockaddr*>(&remote_addr), &sin_size);
    if (new_sd < 0) {
        std::string errmsg("accept: ");
        errmsg += ::strerror(errno);
        throw std::runtime_error(errmsg);
    }

    return SocketStream(new_sd);
}

void Socket::connect(const char* host, const char* port) {
}

