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


void get_input(std::string prompt) {
    std::string buf;
    while (true) {
        std::getline(std::cin, buf);
        if (!buf.empty()) {
            outgoing.enqueue(prompt + buf);
            if (buf != "\\quit")
                std::cout << prompt << std::flush;
        }
    }
}

void handle_clients(std::string prompt) {
    while (true) {
        // Sleep for 1 millisecond to avoid consuming too much CPU time
        ::usleep(1000);

        bool received = false;
        std::string out_message;
        std::string in_message;
        
        // Get one outgoing server message (if any)
        if (!outgoing.empty()) {
            out_message = outgoing.dequeue();
            
            // If \quit is entered, disconnect all clients
            if (out_message == prompt + "\\quit") {
                // Clear queued messages
                while (!outgoing.empty())
                    outgoing.dequeue();
                
                // Disconnect all clients
                std::lock_guard<std::mutex> guard(clients_mutex);
                auto it = clients.begin();
                while (it != clients.end()) {
                    it->close();
                    it = clients.erase(it);
                }
                
                // Skip rest of loop
                continue;
            }
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
                    // Message received -- set flag
                    received = true;
                    
                    // Display message on next line
                    std::cout << std::endl << in_message << std::endl;
                    
                    // Send to each connected client
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
        
        // Redisplay prompt if message was received
        if (received) {
            std::cout << prompt << std::flush;
        }
    }
}

int main(int argc, char* argv[]) {
    // Verify command line arguments
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " listen_port" << std::endl;
        exit(1);
    }

    // Prompt the user for their handle
    std::string handle;
    while (handle.empty() || handle.size() > 10) {
        std::cout << "Enter a handle up to 10 characters: ";
        std::getline(std::cin, handle);
    }
    
    Socket s = Socket();

    // Start listening for connections
    try {
        s.listen(argv[1]);
        std::cout << "Waiting for connections on port " 
            << argv[1] << "..." << std::endl;
    }
    catch (const std::runtime_error& ex) {
        std::cout << ex.what() << std::endl;
        exit(1);
    }

    // Start input thread
    std::thread input_thread (get_input, handle + "> ");
    
    // Start client handling thread
    std::thread client_thread (handle_clients, handle + "> ");

    // Accept incoming connections until interrupt
    while (true) {
        try {
            SocketStream ss = s.accept();
            std::cout << std::endl 
                << "Received connection from: " << s.get_dest_host() << ":" 
                << s.get_dest_port() << std::endl
                << handle << "> " << std::flush;

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
