#include <iostream>
#include <stdexcept>
#include <unistd.h>

#include "Socket.hpp"
#include "SocketStream.hpp"

int main(int argc, char* argv[]) {

    Socket s = Socket();

    try {
        s.listen("51423");
        SocketStream ss = s.accept();
        std::cout << "Received connection from: "
            << s.get_dest_host() << ":" 
            << s.get_dest_port() << std::endl;

        std::string message;
        while (ss.recv(message)) {
            if (!message.empty())
                std::cout << message << std::endl;
            std::getline(std::cin, message);
            ss.send(message);
        }
    }
    catch (const std::runtime_error& ex) {
        std::cout << ex.what() << std::endl;
    }

    return 0;
}
