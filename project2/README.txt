=================================================
Author:      David Rigert
Date:        5/22/2016
Class:       CS372 Spring 2016
Description: README for CS372 Project #2
=================================================

=================================================
TCP File Transfer Server
=================================================
The ftserve program listens for TCP connections on a specific port.
Whenever a client connects, the server displays the client hostname
(if available) and port number.
The server then waits for the client to send a command, parses it,
and initiates a separate data connection with the client to send the
requested data.
The server closes the connection after the client acknowledges receipt
of the data.

BUILD INSTRUCTIONS:
1. Copy ftserve.cpp and the makefile to the same directory.
2. Type 'make' (without the quotes).
   (To build just the server, type 'make ftserve')

USAGE INSTRUCTIONS:
1. Start the server with the following syntax:
   ./ftserve <port_num>
2. To shut down the server, press Ctrl+C.
   This disconnects any connected clients and aborts all transfers.

=================================================
TCP File Transfer Client
=================================================
The ftclient program connects to a server on a specific address and port,
and sends a specific command to the server. The client then waits for the
server to send the requested data on the specified data port.
If the client requests a directory listing or change of directory,
the received data is displayed in the terminal window.
If the client requests a file transfer, the data is saved to a file.
The filename will have a number added to the end if a file with the
same name already exists.

BUILD INSTRUCTIONS:
1. Copy ftclient.py and the makefile to the same directory.
2. Type 'make' (without the quotes).
   (To build just the client, type 'make ftclient')

USAGE INSTRUCTIONS:
1. Start the client with the following syntax:
   ./ftclient server_host server_port (-l | -g FILENAME | -c DIRNAME) data_port
  * To list files, type the following:
    ./ftclient server_host server_port -l data_port
  * To get a file, type the following:
    ./ftclient server_host server_port -g FILENAME data_port
  * To change the server's directory, type the following:
    ./ftclient server_host server_port -c DIRNAME data_port
2. For help, type the following:
   ./ftclient -h

=================================================
Extra Credit Features
=================================================
1. The server is multithreaded and accepts connections from multiple clients
   at the same time. Each client is handled in a separate thread.
2. A client can use the -c DIRNAME command to request a change of directory
   in the server. This affects the server's current directory for all clients.
3. The server and client support the transfer to binary files as well as text.
4. The server handles SIGINT and gracefully shuts down the listen socket and
   display thread.
5. The directory list prepends each entry with a letter to indicate whether
   it is a directory, a file, a link, a socket, or a pipe.
6. When receiving a file, the client automatically appends a number between
   the filename and the extension (if any) if a file with that name already
   exists. The number is incremented each time an additional copy is
   downloaded.
