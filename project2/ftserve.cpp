/*********************************************************\
* Author:       David Rigert
* Class:        CS372 Spring 2016
* Assignment:   Project 2
* File:         ftserve.cpp
* Description:  A file transfer server written in C++.
*
*               This program accepts TCP connections from a ftclient client,
*               receives a command, and sends a message or file data in response
*               as long as the program is running.
*               Multiple ftclient connections can be handled at the same time.
*
*               The command line syntax is as follows:
*
*                   ftserve port
*
*               This program takes the following arguments:
*               - port      -- The TCP port on which to wait for client
*                              connections.
\*********************************************************/
#include <algorithm>
#include <atomic>
#include <iostream>
#include <mutex>
#include <queue>
#include <cstring>
#include <string>
#include <sstream>
#include <thread>

#include <cerrno>
#include <exception>
#include <stdexcept>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>

// Queue length for listen sockets
#define SOCKET_CONNECTION_QUEUE 10
// Receive buffer size
#define BUFFER_SIZE 501

// String for -l command
#define LIST_COMMAND "LIST"
// String for -g command
#define GET_COMMAND "GET"
// String for acknowledgement
#define ACK_COMMAND "ACK"


/*========================================================*
 * Class declarations
 *========================================================*/
class Socket {
public:

    /**
    * Constructor.
    * Optionally sets the file descriptor and listen queue length.
    *
    *  sd          The initial socket descriptor value.
    *  queuelen    The queue size for incoming connections. 
    */
    Socket(int sd = -1, int queuelen = SOCKET_CONNECTION_QUEUE) {
        _queue_len = queuelen;
        _sd = sd;
        _info = nullptr;
    }

    /**
    * Destructor.
    * Closes the socket and frees any memory allocated for address info.
    */
    ~Socket() {
        ::close(_sd);
        if (_info != nullptr) ::freeaddrinfo(_info);
    }

    /**
    * Configures the socket to listen for connections on the specified port.
    *
    * This function throws a runtime_error exception if any of the steps fail.
    *
    *  port   The port to listen for connections on.
    */
    void listen(const char* port) {
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
            ::close(_sd);
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

    /**
     * Accepts an incoming connection.
     *
     * This function throws a runtime_error exception if any of the steps fail.
     *
     * Returns a Socket for the new connection.
     */
    Socket accept() {
        int new_sd;
        struct sockaddr_storage remote_addr;
        socklen_t sin_size = sizeof(remote_addr);

        new_sd = ::accept(_sd, reinterpret_cast<struct sockaddr*>(&remote_addr), &sin_size);
        if (new_sd < 0) {
            std::string errmsg("accept: ");
            errmsg += ::strerror(errno);
            throw std::runtime_error(errmsg);
        }

        // Create a new Socket based on the returned descriptor
        Socket s_client (new_sd);
        
        // Get the remote IP and port information
        s_client.get_remote_addr(reinterpret_cast<struct sockaddr*>(&remote_addr));

        return s_client;
    }

    /**
     * Connects to the specified host on the specified port.
     *
     * This function throws a runtime_error exception if any of the steps fail.
     *
     *  host    The hostname to establish a connection with.
     *  port    The port number to connect to.
     */
    void connect(const char* host, const char* port) {
        struct addrinfo hints;
        struct addrinfo *current = nullptr;
        int retval;
        std::string errmsg;

        // Zero-initialize and set addrinfo structure
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6, whichever is available
        hints.ai_socktype = SOCK_STREAM; // Non-blocking TCP

        // Look up the remote host address info
        retval = ::getaddrinfo(host, port, &hints, &_info);
        if (retval != 0) {
            errmsg = "getaddrinfo: ";
            errmsg += ::gai_strerror(retval);
            throw std::runtime_error(errmsg);
        }

        // Loop through address structure results until connect succeeds
        for (current = _info; current != NULL; current = current->ai_next) {
            // Attempt to open a socket based on the remote host's address info
            _sd = ::socket(current->ai_family, current->ai_socktype, current->ai_protocol);
            if (_sd == -1) {
                continue; // Try next on error
            }
            
            if (::connect(_sd, current->ai_addr, current->ai_addrlen) == -1) {
                ::close(_sd);
                continue;
            }
            
            // connect succeeded
            break;
        }
        
        // Throw an exception if connect failed on all returned addresses
        if (current == NULL) {
            errmsg = "connect: No valid address found";
            throw std::runtime_error(errmsg);
        }

        // Get the remote IP and port information
        get_remote_addr(reinterpret_cast<struct sockaddr*>(current->ai_addr));
        // Store the original hostname
        _hostname.assign(host);

        // Free memory used by remote host's address info
        ::freeaddrinfo(_info);
        _info = nullptr;
    }

    /**
     * Sends the specified data to the connected host.
     *
     * This function continues to send until all data has been sent
     * or the socket is closed.
     *
     *  data    The data to send over the socket.
     *
     * Returns whether the socket is still open.
     */
    bool send(std::string data) {
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

    /**
     * Receives the specified amount of data from the connected host.
     *
     * This function blocks until the specified amount of data is received,
     * or the socket is closed.
     *
     *  buffer  A string buffer to store the received data.
     *  len     The length of the data to receive.
     *
     * Returns whether the socket is still open.
     */
    bool recv(std::string& buffer, ssize_t len) {
        ssize_t bytes;
        char buf[BUFFER_SIZE];
        buffer.clear();
        
        // Keep trying until 'len' bytes are received
        while (buffer.size() < len) {
            bytes = ::recv(_sd, buf, BUFFER_SIZE - 1, 0);
            
            if (bytes == 0) {
                // socket was closed, return false
                return false;
            } else if (bytes == -1) {
                if (errno == EINTR) {
                    // Try again if interrupted by signal
                    continue;
                } else {
                    // Otherwise, throw exception
                    std::string errmsg("recv: ");
                    errmsg += ::strerror(errno);
                    throw std::runtime_error(errmsg);
                }
            }
            // Append null terminator
            buf[bytes] = '\0';
            // Concatenate to end of string buffer
            buffer.append(buf);
        }
    }

    /**
     * Receives BUFFER_SIZE - 1 worth of data from the connected host.
     *
     * This function blocks until some data is received,
     * or the socket is closed.
     *
     *  buffer  A string buffer to store the received data.
     *
     * Returns whether the socket is still open.
     */
    bool recv(std::string& buffer) {
        ssize_t bytes;
        char buf[BUFFER_SIZE];
        buffer.clear();
        
        // Keep trying until something is received
        while (true) {
            bytes = ::recv(_sd, buf, BUFFER_SIZE - 1, 0);

            if (bytes == 0) {
                // socket was closed, return false
                return false;
            } else if (bytes == -1) {
                if (errno == EINTR) {
                    // Try again if interrupted by signal
                    continue;
                } else {
                    // Otherwise, throw exception
                    std::string errmsg("recv: ");
                    errmsg += ::strerror(errno);
                    throw std::runtime_error(errmsg);
                }
            }
            
            // Append null terminator
            buf[bytes] = '\0';
            // Add to string buffer
            buffer.append(buf);
            // One data block received, return to caller
            return true;
        }
    }

    /**
     * Closes the socket.
     *
     * This function immediately closes the underlying socket descriptor.
     * After this function is called, the object can no longer be used for 
     * sending or receiving until another connection is established.
     */
    void close() {
        ::close(_sd);
    }

    std::string get_hostname() { return _hostname; }
    std::string get_host_ip() { return _host_ip; }
    std::string get_port() { return _port; }

private:

    int _sd;                // Underlying socket descriptor
    int _queue_len;         // Max incoming connections to queue
    struct addrinfo* _info; // Used for address info lookup
    std::string _hostname;  // Hostname of connected client
    std::string _host_ip;   // IP of connected client
    std::string _port;      // Port number of connected client

    /**
     * Gets the remote host IP address and port.
     *
     * This function extracts the information for both IPv4 and IPv6 connections.
     *
     *  sa      The sockaddr struct to parse for the information.
     */
    void get_remote_addr(struct sockaddr* sa) {
        char s[INET6_ADDRSTRLEN]; // temp storage buffer for IP address
        void* in_addr = nullptr;

        // IPv4 connection
        if (sa->sa_family == AF_INET) {
            struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(sa);
            in_addr = reinterpret_cast<void*>(&addr->sin_addr);
            _port.assign(std::to_string(ntohs(addr->sin_port)));
        }
        // IPv6 connection
        else {
            struct sockaddr_in6* addr = reinterpret_cast<struct sockaddr_in6*>(sa);
            in_addr = reinterpret_cast<void*>(&addr->sin6_addr);
            _port.assign(std::to_string(ntohs(addr->sin6_port)));
        }

        ::inet_ntop(sa->sa_family, in_addr, s, sizeof(s));
        _host_ip.assign(s);
    }
}; // End of Socket class

/*========================================================*
 * Forward declarations
 *========================================================*/
void handle_client(Socket);
void display_output();
std::vector<std::string> get_files_in_dir(const char*);

/*========================================================*
 * Global variables
 *========================================================*/
// A queue of terminal output from handle_client to display in server terminal
std::queue<std::string> output;
// A mutex for ensuring thread-safe access to terminal output queue
std::mutex output_mutex;

// An atomic to signal that the server is shutting down
std::atomic<bool> is_shutting_down(false);

/*========================================================*
 * main function
 *========================================================*/
int main(int argc, char* argv[]) {
    // Verify command line arguments
    if (argc != 2) {
        std::cout << "usage: " << argv[0] << " listen_port" << std::endl;
        exit(1);
    }

    // Instantiate a Socket object for listening
    // The Socket class member functions abstract away the details
    // of the socket library.
    Socket s = Socket();

    // Start listening for connections
    try {
        s.listen(argv[1]);
        std::cout << "Server open on " << argv[1] << std::endl;
    }
    catch (const std::runtime_error& ex) {
        // Exit with an error if any exceptions occur during listen/bind
        std::cout << ex.what() << std::endl;
        exit(1);
    }

    // Start a thread to handle the display of terminal output from
    // connected clients.
    std::thread output_thread (display_output);
    
    // Accept incoming control connections until interrupt
    while (true) {
        try {
            // The SocketStream class abstracts away the details of sending
            // and receiving data over a socket.
            Socket s_client = s.accept();
            std::cout << "Connection from " << s_client.get_hostname() << "." 
                << std::endl;

            // Spawn a new thread to handle the connected client.
            // Detach the thread because it runs independently.
            std::thread th(handle_client, s_client);
            th.detach();
        }
        catch (const std::runtime_error& ex) {
            std::cout << ex.what() << std::endl;
        }
    }
    
    // Join the output thread before terminating.
    output_thread.join();

    return 0;
}

/**
 * Handles a client through the specified socket.
 *
 * The socket is for the control connection.
 * This function waits for the client to send a command through the socket,
 * parses the command, and then sends a message or data in response.
 * If the client requests the transfer of a file, this function
 * establishes a new connection with the client for sending file data.
 *
 * This function is intended to be run in a separate thread so that new
 * clients can continue to be accepted in the main thread.
 *
 *  s  The Socket for the connected client.
 */
void handle_client(Socket s) {
    std::string input;
    std::ostringstream msg;
    
    // Get command from client
    if (!s.recv(input)) {
        // Socket closed; client disconnected
        std::lock_guard<std::mutex> guard(output_mutex);
        msg << s.get_host_ip() << " disconnected" << std::endl;
        output.emplace(msg.str().c_str());
        return;
    }
    
    // Command received; verify command
    // Put input in stringstream for easy parsing
    std::istringstream instream (input);
    std::string cmd;
    std::getline(instream, cmd);
    std::vector<std::string> files;
    
    if (cmd == LIST_COMMAND) {
        // Get a list of files in the current directory
        try {
            files = get_files_in_dir(".");
        }
        catch (const std::runtime_error& ex) {
            std::cout << ex.what() << std::endl;
        }
        
        // Join the filenames into a single string for sending
        std::stringstream fss;
        std::for_each(files.begin(), files.end(), [&fss] (const std::string& str) 
            { fss << str << std::endl; });
        // Send the size of the data to be sent
        s.send(std::to_string(fss.str().length()));
        // Wait for acknowledgement
        if (!s.recv(input)) {
            // Socket closed; client disconnected
            std::lock_guard<std::mutex> guard(output_mutex);
            msg << s.get_host_ip() << " disconnected" << std::endl;
            output.emplace(msg.str().c_str());
            return;
        }
        // Send the contents of the CWD to the client over s
        s.send(fss.str());
    } else if (cmd == GET_COMMAND) {
        // Attempt to send the specified file to the client over new socket
    } else {
        // Invalid command; send error message over s
        s.send(std::string("INVALID COMMAND"));
    }
    
}

/**
 * Displays all output messages queued up by client connnections.
 *
 * This function is intended to be run in a separate thread.
 * It uses thread-safe access to a global queue to avoid having
 * multiple clients attempting to write to cout simultaneously.
 */
void display_output() {
    // Keep running until server shuts down
    while (!is_shutting_down.load()) {
        // Lock the output queue mutex for thread safety
        std::lock_guard<std::mutex> guard(output_mutex);
        // Display all queued output messages
        while (!output.empty()) {
            std::cout << output.front() << std::endl;
            output.pop();
        }
        
        // Sleep for 1 ms to avoid consuming too much CPU time
        ::usleep(1000);
    }
}

std::vector<std::string> get_files_in_dir(const char* name) {
    std::vector<std::string> files;
    struct dirent* entry = nullptr;
    
    // Attempt to open the specified directory
    DIR *d = opendir(name);
    if (d == nullptr) {
        // Throw exception if opening directory fails
        std::string errmsg("recv: ");
        errmsg += ::strerror(errno);
        throw std::runtime_error(errmsg);
    }
    
    // Set errno to 0 to detect any readdir errors
    errno = 0;
    // Attempt to read all entries and add them to file list
    for (entry = readdir(d); entry != nullptr; entry = readdir(d)) {
        files.push_back(std::string(entry->d_name));
    }
    
    if (errno != 0) {
        // Some error occurred. Throw an exception
        std::string errmsg("recv: ");
        errmsg += ::strerror(errno);
        throw std::runtime_error(errmsg);
    }
    
    // Everything worked if execution reaches here
    return files;
}