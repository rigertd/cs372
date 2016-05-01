#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "SafeQueue.hpp"
#include "Socket.hpp"
#include "SocketStream.hpp"

SafeQueue<std::string> outgoing;

std::vector<SocketStream> clients;
std::mutex clients_mutex;


void get_input() {
    std::string buf;
    while (true) {
        std::getline(std::cin, buf);
        if (!buf.empty()) {
            outgoing.enqueue(buf);
        }
    }
}

void handle_clients() {
    while (true) {
        // Sleep for 1 millisecond
        ::usleep(1000);

        std::string out_message;
        std::string in_message;
        
        // Get one outgoing server message (if any)
        if (!outgoing.empty()) {
            out_message = outgoing.dequeue();
        }
        
        // Handle sending and receiving of messages from clients
        std::lock_guard<std::mutex> guard(clients_mutex);
        auto it = clients.begin();
        while (it != clients.end()) {
            // Send the server message (if any)
            if (!out_message.empty())
                it->send(out_message);
            // Receive any messages from client
            if (it->recv(in_message)) {
                if (!in_message.empty()) {
                    // Message received -- display to console
                    std::cout << in_message << std::endl;
                    // and send to each connected client
                    auto it2 = clients.begin();
                    while (it2 != clients.end()) {
                        if (it != it2)
                            it2->send(in_message);
                        ++it2;
                    }
                }
                
                ++it;
            }
            else {
                // Socket closed -- remove client
                it = clients.erase(it);
            }
        }
    }
}

int main(int argc, char* argv[]) {

    Socket s = Socket();

    // Start listening for connections
    try {
        s.listen("51423");
        std::cout << "Waiting for connections..." << std::endl;
    }
    catch (const std::runtime_error& ex) {
        std::cout << ex.what() << std::endl;
    }

    // Start input thread
    std::thread input_thread (get_input);
    
    // Start client handling thread
    std::thread client_thread (handle_clients);

    // Accept incoming connections until interrupt
    while (true) {
        try {
            SocketStream ss = s.accept();
            std::cout << "Received connection from: "
                << s.get_dest_host() << ":" 
                << s.get_dest_port() << std::endl;

            // Add new socket to list of currently connected clients
            std::lock_guard<std::mutex> guard(clients_mutex);
            clients.push_back(ss);
        }
        catch (const std::runtime_error& ex) {
            std::cout << ex.what() << std::endl;
        }

    }

    return 0;
}
