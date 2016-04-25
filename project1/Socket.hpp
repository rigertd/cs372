#pragma once

#include <string>
#include <sys/types.h>
#include <sys/socket.h> /* SOCK_STREAM */

#ifndef SOCKET_CONNECTION_QUEUE
#define SOCKET_CONNECTION_QUEUE 10
#endif

class SocketStream;

class Socket {
    public:
        Socket(int queuelen = SOCKET_CONNECTION_QUEUE);
        ~Socket();
        
        void listen(const char* port);
        SocketStream accept();
        SocketStream connect(const char* host, const char* port);
        
        std::string get_dest_host() { return _dest_host; }
        std::string get_dest_port() { return _dest_port; }

    private:
        int _sd;
        int _queue_len;
        std::string _dest_host;
        std::string _dest_port;
        struct addrinfo* _info;
        
        void store_remote_addr(struct sockaddr* sa);
};
