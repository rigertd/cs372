#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h> /* SOCK_STREAM */

#ifndef SOCKET_CONNECTION_QUEUE
#define SOCKET_CONNECTION_QUEUE 10
#endif

class Socket {
    public:
        Socket(int queuelen = SOCKET_CONNECTION_QUEUE);
        ~Socket();
        
        void listen(const char* port);
        Socket accept();
        void connect(const char* host, const char* port);

    private:
        int _sd;
        int _queue_len;
        std::string _dest_host;
        std::string _dest_port;
        struct addrinfo* _info
};