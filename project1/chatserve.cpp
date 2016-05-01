/*********************************************************\
* Author:       David Rigert
* Class:        CS372 Spring 2016
* Assignment:   Project 1
* File:         chatserve.cpp
* Description:  A multiuser chat server written in C++.
*
*               This program accepts TCP connections from chatclient clients
*               and adds them to a chat room that persists as long as
*               the program is running.
*               Messages received from clients are displayed in the console
*               and forwarded to all other connected clients.
*
*               The command line syntax is as follows:
*
*                   chatserve port
*
*               This program takes the following arguments:
*               - port      -- The TCP port on which to wait for client
*                              connections.
\*********************************************************/
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "Socket.hpp"
#include "SocketStream.hpp"

/*========================================================*
 * Global variables
 *========================================================*/
// A queue for outgoing messages typed in the server console
std::queue<std::string> outgoing;
// A mutex for ensuring thread-safe access to outgoing queue
std::mutex outgoing_mutex;

// A list of sockets for all connected clients
std::vector<SocketStream> clients;
// A mutex for ensuring thread-safe access to socket list
std::mutex clients_mutex;

/*========================================================*
 * Forward declarations
 *========================================================*/
void get_input(std::string);
void handle_clients(std::string);

/*========================================================*
 * main function
 *========================================================*/
int main(int argc, char* argv[]) {
    // Verify command line arguments
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " listen_port" << std::endl;
        exit(1);
    }

    // Prompt the user for their handle
    // Keep prompting until a valid handle is entered
    std::string handle;
    while (handle.empty() || handle.size() > 10) {
        std::cout << "Enter a handle up to 10 characters: ";
        std::getline(std::cin, handle);
    }

    // Instantiate a socket for listening
    Socket s = Socket();

    // Start listening for connections
    try {
        s.listen(argv[1]);
        std::cout << "Waiting for connections on port "
            << argv[1] << "..." << std::endl;
    }
    catch (const std::runtime_error& ex) {
        // Exit with an error if any exceptions occur during listen/bind
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
                << "Accepted connection from: " << ss.get_hostname() << ":"
                << ss.get_port() << std::endl;

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

/**
 * Gets input from stdin and adds it to the outgoing message queue.
 *
 * This function is intended to be run in a separate thread for non-blocking
 * input on stdin. It displays a prompt that includes the server user's handle.
 * This function runs until the program terminates.
 *
 *  prompt  The prompt string to display.
 */
void get_input(std::string prompt) {
    std::string buf;
    while (true) {
        std::getline(std::cin, buf);
        if (!buf.empty()) {
            std::lock_guard<std::mutex> guard(outgoing_mutex);
            outgoing.emplace(buf);
            if (buf != "\\quit")
                std::cout << prompt << std::flush;
        }
    }
}

/**
 * Sends and receives messages to and from all connected clients.
 *
 * Any message entered in the server console is prepended with
 * the server user's handle and sent to all connected clients.
 * Any message received from a client is displayed in the server console
 * and sent to all other connected clients.
 *
 * This function is intended to be run in a separate thread so that new
 * clients can continue to be accepted in the main thread.
 *
 *  prompt  The prompt string to display and prepend to any entered text.
 */
void handle_clients(std::string prompt) {
    while (true) {
        // Sleep for 1 millisecond to avoid consuming too much CPU time
        ::usleep(1000);

        bool received = false;
        std::string out_message;
        std::string in_message;

        // Get one outgoing server message (if any)
        if (!outgoing.empty()) {
            std::lock_guard<std::mutex> guard(outgoing_mutex);
            out_message = outgoing.dequeue();

            // If \quit is entered, disconnect all clients
            if (out_message == "\\quit") {
                // Clear queued messages
                while (!outgoing.empty())
                    outgoing.dequeue();

                // Disconnect all clients
                std::lock_guard<std::mutex> guard(clients_mutex);
                auto it = clients.begin();
                while (it != clients.end()) {
                    it->close();
                    std::cout << std::endl
                        << it->get_hostname() << ":" << it->get_port()
                        << " disconnected" << std::endl;
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
                it->send(prompt + out_message);
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
                std::cout << std::endl
                    << it->get_hostname() << ":" << it->get_port()
                    << " disconnected" << std::endl;
                it = clients.erase(it);
            }
        }

        // Redisplay prompt if message was received
        if (received) {
            std::cout << prompt << std::flush;
        }
    }
}
