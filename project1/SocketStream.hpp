#pragma once

#include <string>

class SocketStream {
    private:
        int _sd;
    public:
        SocketStream(int sock_desc);
        
        bool send(std::string data);
        bool recv(std::string& buffer);
};
