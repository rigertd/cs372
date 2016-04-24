#pragma once

#include <string>

class SocketStream {
    private:
        int _sd;
    public:
        SocketStream(int sock_desc)
            : _sd(sock_desc) {}
        
        int send(std::string data);
        int recv(std::string& buffer);
};