=================================================
Author:      David Rigert
Date:        5/1/2016
Class:       CS372 Spring 2016
Description: README for CS372 Project #1
=================================================

=================================================
TCP Multi-User Chat Server
=================================================
The chat server listens for TCP connections on a specific port.
Whenever a client connects, a connection message displays the client's
IP address and port number.
The server user will be prompted for input only when at least one client
is connected.

BUILD INSTRUCTIONS:
1. Extract all source files from rigertd.project1.zip.
2. Type 'make' (without the quotes).

USAGE INSTRUCTIONS:
1. Start the server with the following syntax:
   ./chatserve <port_num>
2. Enter the server user's handle at the prompt.
   The handle must be between 1 and 10 characters.
3. Wait for at least one client to connect
4. When a client is connected, type the message to send and press Enter.
   Note that messages are sent to all connected clients.
5. When a client sends a message, it will appear in the terminal.
6. Type '\quit' (without the quotes) to disconnect all clients.

=================================================
TCP Chat Client
=================================================
The chat client connects to one chat server on a specific address and port.
The connection is not actually established until the first message is sent.
Messages sent to the server are displayed on the server and on all other
connected clients.

USAGE INSTRUCTIONS:
1. Start the client with the following syntax:
   ./chatclient <host_name> <port_num>
2. Enter the client user's handle at the prompt.
   The handle must be between 1 and 10 characters.
3. Type a message at the prompt and press Enter to establish a connection
   and send the message to the server.
4. Continue typing and sending messages as desired.
5. When the server or another client sends a message, 
   it will appear in the terminal.
6. Type '\quit' (without the quotes) to disconnect from the server.

=================================================
Extra Credit Features
=================================================
1. The server accepts connections from unlimited clients 
   (memory permitting).
2. All messages are sent to and received from all connected clients.
3. The server uses separate threads for handling input and routing messages.
4. Clients and the server can send and receive messages at any time.
