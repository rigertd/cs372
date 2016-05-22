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
#include <fstream>
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
#include <sys/stat.h>
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
 * Class declaration
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
        if (_info != nullptr) ::freeaddrinfo(_info);
    }

    // Function prototypes--see implementations at bottom for descriptions
    void listen(const char* port);
    Socket accept();
    void connect(const char* host, const char* port);
    bool send(std::string data);
    bool send(std::ifstream& data);
    bool recv(std::istringstream& buffer, ssize_t len);
    bool recv(std::istringstream& buffer);
    void close();

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

    void get_remote_addr(struct sockaddr* sa);
}; // End of Socket class

/*========================================================*
 * Forward declarations
 *========================================================*/
void handle_client(Socket);
void display_output();
std::vector<std::string> get_files_in_dir(const char*);
std::string get_line(std::istringstream&);
size_t get_file_size(std::ifstream&);
void print_message(const std::string&);

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
            std::cout << "Connection from " << s_client.get_host_ip() << "." 
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
    std::istringstream inbuf;
    std::ostringstream msg;
    std::string cmd;
    
    // Get command from client
    if (!s.recv(inbuf)) {
        // Socket closed; client disconnected
        msg << s.get_host_ip() << " disconnected" << std::endl;
        print_message(msg.str());
        return;
    }
    
    // Command received
    // Extract first token to get command
    inbuf >> cmd;
    // Verify command
    if (cmd == LIST_COMMAND) {
        // Create vector to store list of files
        std::vector<std::string> files;
        
        // Get a list of files in the current directory
        try {
            files = get_files_in_dir(".");
        }
        catch (const std::runtime_error& ex) {
            msg << ex.what() << std::endl;
            print_message(msg.str());
        }
        
        // Join the filenames into a single string for sending
        std::stringstream ss;
        std::for_each(files.begin(), files.end(), [&ss] (const std::string& str) 
            { ss << str << std::endl; });
        // Send the size of the data to be sent
        s.send(std::to_string(ss.str().length()));
        // Wait for acknowledgement
        if (!s.recv(inbuf)) {
            // Socket closed; client disconnected
            msg << s.get_host_ip() << " disconnected" << std::endl;
            print_message(msg.str());
            return;
        }
        cmd = get_line(inbuf);
        if (cmd == ACK_COMMAND) {
            // Send the contents of the CWD to the client over s
            s.send(ss.str());
        }
    } else if (cmd == GET_COMMAND) {
        // Get the data port from the rest of the first line
        std::string port_str = get_line(inbuf);
        int data_port = std::stoi(port_str);
        // Get the file name from the next line
        std::string filename = get_line(inbuf);
        // Verify that file exists
        struct stat sb;
        if (stat(filename.c_str(), &sb) == -1) {
            switch (errno) {
            case EACCES:
                // Access denied. Send an appropriate error message
                msg << "Access denied. Sending error message to "
                    << s.get_host_ip() << std::endl;
                print_message(msg.str());
                s.send(std::string("ACCESS DENIED"));
                break;
            case ENOENT:
                // File not found. Send an appropriate error message
                msg << "File not found. Sending error message to "
                    << s.get_host_ip() << std::endl;
                print_message(msg.str());
                s.send(std::string("FILE NOT FOUND"));
                break;
            default:
                // Other error. Send a generic error message
                msg << "Some other error occurred. Sending error message to "
                    << s.get_host_ip() << std::endl;
                print_message(msg.str());
                s.send(std::string("ERROR OCCURRED"));
                break;
            }
            s.close();
            return;
        }
        
        // Send an error message if the client requested a directory
        if (S_ISDIR(sb.st_mode)) {
            msg << "Client requested a directory. Sending error message to "
                << s.get_host_ip() << std::endl;
            print_message(msg.str());
            s.send(std::string("CANNOT SEND DIRECTORY"));
            s.close();
            return;
        }
        
        // Open the file
        std::ifstream file(filename.c_str(), std::ios::binary);
        // Only send the file if it can be read
        if (file.good()) {
            msg << "Sending " << filename << " to " << s.get_host_ip()
                << std::endl;
            print_message(msg.str());
            // Send the file size over the control connection
            s.send(std::to_string(get_file_size(file)));
        }
        else {
            // Some error occurred. Send a generic error message
            msg << "File read error. Sending error message to "
                << s.get_host_ip() << std::endl;
            print_message(msg.str());
            s.send(std::string("FILE READ ERROR"));
            s.close();
            return;
        }
        
        // Establish connection to client data port
        std::cout << "Attempting to connect to port " << data_port << std::endl;
        // Attempt to send the specified file to the client over new socket
        // if (!s.send(file)) {
            // // The socket was closed before the file finished sending
            // std::lock_guard<std::mutex> guard(output_mutex);
            // msg << "Client disconnected before transfer was complete."
                // << std::endl;
            // output.emplace(msg.str().c_str());
        // }
    } else {
        // Invalid command; send error message over s
        s.send(std::string("INVALID COMMAND"));
    }
    s.close();
}

/**
 * Prints a message to the terminal window on the server in a thread-safe
 * manner.
 *
 *  msg     The message to print.
 */
void print_message(const std::string& msg) {
    std::lock_guard<std::mutex> guard(output_mutex);
    output.emplace(msg.c_str());
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
        // Sleep for 10 ms to avoid consuming too much CPU time
        ::usleep(10000);
        // Lock the output queue mutex for thread safety
        std::lock_guard<std::mutex> guard(output_mutex);
        // Display all queued output messages
        while (!output.empty()) {
            std::cout << output.front() << std::endl;
            output.pop();
        }
        
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
        // Skip the current and previous directory entries
        if (::strcmp(entry->d_name, "..") != 0 && ::strcmp(entry->d_name, ".") != 0)
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

/**
 * Gets a line of text from the specified string, removes it
 * from the string and returns it as a separate string.
 *
 *  source  The string to get a line of text from.
 */
std::string get_line(std::istringstream& source) {
    std::string temp;
    std::getline(source, temp);
    size_t pos = temp.find_last_not_of("\r\n\t ");
    return temp.substr(0, pos + 1);
}

/**
 * Gets the number of bytes available in an ifstream.
 *
 *  fs  The ifstream to get the size of.
 *
 * Returns the number of bytes in the specified ifstream.
 */
size_t get_file_size(std::ifstream& fs) {
    size_t length = -1;
    if (fs) {
        fs.seekg(0, fs.end);
        length = fs.tellg();
        fs.seekg(0, fs.beg);
    }
    return length;
}

/**
 * Configures the socket to listen for connections on the specified port.
 *
 * This function throws a runtime_error exception if any of the steps fail.
 *
 *  port   The port to listen for connections on.
 */
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
Socket Socket::accept() {
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
void Socket::connect(const char* host, const char* port) {
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
bool Socket::send(std::string data) {
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
 * Sends the specified binary data to the connected host.
 *
 * This function continues to send until all data has been sent
 * or the socket is closed.
 *
 *  data    The binary data to send over the socket. The stream must be open.
 *
 * Returns whether the socket is still open.
 */
bool Socket::send(std::ifstream& data) {
    ssize_t bytes = 0;
    size_t length = 0;
    size_t sent = 0;
    
    // Allocate a buffer for reading data from the stream before sending
    char buf[BUFFER_SIZE];
    
    // Get length of the file
    length = get_file_size(data);

    // Send until there is nothing left to send
    while (sent < length) {
        memset(buf, 0, BUFFER_SIZE);
        // Load the file data into the buffer
        data.read(buf, BUFFER_SIZE);
        // Calculate the amount to send in this iteration
        size_t to_send = length - sent < BUFFER_SIZE ? length - sent : BUFFER_SIZE;
        // Send the data over the socket
        bytes = ::send(_sd, buf, to_send, 0);
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

        // Update the number of bytes sent
        sent += bytes;
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
 *  buffer  An istringstream buffer to store the received data.
 *  len     The length of the data to receive.
 *
 * Returns whether the socket is still open.
 */
bool Socket::recv(std::istringstream& buffer, ssize_t len) {
    ssize_t bytes;
    char buf[BUFFER_SIZE];
    std::ostringstream received;
    buffer.str("");
    buffer.clear();
    
    // Keep trying until 'len' bytes are received
    while (buffer.gcount() < len) {
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
        // Add to stringstream buffer
        received << buf;
    }
    
    // Update input buffer with received data
    buffer.str(received.str());
    
    return true;
}

/**
 * Receives BUFFER_SIZE - 1 worth of data from the connected host.
 *
 * This function blocks until some data is received,
 * or the socket is closed.
 *
 *  buffer  An istringstream buffer to store the received data.
 *
 * Returns whether the socket is still open.
 */
bool Socket::recv(std::istringstream& buffer) {
    ssize_t bytes;
    char buf[BUFFER_SIZE];
    buffer.str("");
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
        // Set stringstream buffer to received data
        buffer.str(buf);
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
void Socket::close() {
    ::close(_sd);
}

/**
 * Gets the remote host IP address and port.
 *
 * This function extracts the information for both IPv4 and IPv6 connections.
 *
 *  sa      The sockaddr struct to parse for the information.
 */
void Socket::get_remote_addr(struct sockaddr* sa) {
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
