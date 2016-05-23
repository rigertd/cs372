#!/usr/bin/env python
"""
chatclient: A simple TCP-based file transfer client program.

Author:     David Rigert
Class:      CS372 Spring 2016
Assignment: Project #2

This program connects to ftserve and either requests a directory listing,
a change of directory, or the transfer of a file.
All files are transferred as binary data.

Command-line syntax:
    ftclient.py server_host server_port (-l | -g FILENAME | -c DIRNAME) data_port

This program takes the following arguments:
    - server_host   -- Hostname or IP address of the server running ftserve
    - server_port   -- Port number that the server is listening on
    - -l, --list    -- Tells the server to send a list of files
    - -g, --get     -- Tells the server to send the specified FILENAME
    - -c, --cd      -- Tells the server to change the directory
    - data_port     -- Port number over which server sends data to client
"""

__author__      = "David Rigert"

import sys
import re
import os
import socket
from argparse import ArgumentParser


def main():
    # Verify the command line arguments
    args = parse_args()

    # Create a socket
    # The Socket class abstracts away the details of the socket library
    control_sock = Socket()

    # Attempt to connect to server
    try:
        control_sock.connect(args.server_host, args.server_port)
    except Exception as ex:
        print("connect: " + ex)
        exit(1)

    # Send command to server along with data port and file/dirname (if any)
    control_sock.send('{0} {1} {2}'.format(args.command, args.data_port, args.filename or args.dirname))

    # Flag to indicate when socket is closed
    is_open = True

    # Get response from server
    is_open, response = control_sock.recv()

    # Check if data size or an error message was returned
    if is_int(response):
        # Data size--listen for incoming connection and send acknowledgement
        data_sock = Socket()
        try:
            data_sock.listen(args.data_port)
        except Exception as ex:
            print(ex)
            exit(1)

        # Send acknowledgement over control connection
        if not control_sock.send("ACK"):
            print('Server closed control connection')
            exit(1)

        # Accept the incoming connection
        try:
            data_sock = data_sock.accept()
        except Exception as ex:
            print(ex)
            exit(1)

        # Display required output in terminal window
        if args.command == 'LIST':
            print('Receiving directory structure from {0}:{1}'.format(args.server_host, args.data_port))
        elif args.command == 'GET':
            print('Receiving "{0}" from {1}:{2}'.format(args.filename, args.server_host, args.data_port))
        else:
            print('Receiving new working directory from {0}:{1}'.format(args.server_host, args.data_port))
        # Receive the amount of data specified in the response
        is_open, data = data_sock.recv_all(int(response))
        if not is_open:
            print('Server closed data connection')
            control_sock.close()
            exit(1)

        # Acknowledge the receipt of the data
        if not control_sock.send('ACK'):
            print('Server closed control connection')
            data_sock.close()
            exit(1)

        # Close the data socket
        data_sock.close()

        # Display data if directory listing, otherwise write to disk
        if args.command == 'LIST':
            sys.stdout.write(data)
            sys.stdout.flush()
        elif args.command == 'GET':
            outfile = get_unique_filename(args.filename)
            f = open(outfile, 'w')
            f.write(data)
            f.close()
            print('File transfer complete.')
        elif args.command == 'CD':
            print(data);

    else:
        # Error message--print it and exit
        print('{0}:{1} says {2}'.format(args.server_host, args.server_port, response))
        control_sock.close()
        exit(1)

def get_unique_filename(name):
    """
    Returns a filename that is unique in the specified directory.

    This function appends a number to the end of the filename and before
    the file extension. The number is incremented until a unique filename
    is found.
    """
    if not os.path.exists(name):
        return name
    else:
        # Match filename followed by number in parens and then optional extension
        m = re.match(r'(.*\()(\d+)(\))(\..+)?', name, re.IGNORECASE)
        if m:
            # Use a list comprehension to skip extension if None
            name = '{0}{1}{2}'.format(m.groups()[0], int(m.groups()[1]) + 1, ''.join(g for g in m.groups()[2:] if g is not None))
        else:
            # Does not have parens w/ number: just add one before the extension
            parts = name.split('.')
            if len(parts) > 1:
                name = '.'.join(parts[:-1]) + '(1).' + parts[-1]
            else:
                name += '(1)'
    # Recursively call until a unique filename is acquired
    return get_unique_filename(name)

def parse_args():
    """
    Configures an ArgumentParser and uses it to parse the command line options.

    Returns an object containing the argument data.
    """
    parser = ArgumentParser(description='Perform transfer operations with ftserve.')
    parser.add_argument('server_host', help='The hostname of the computer running ftserve.')
    parser.add_argument('server_port', help='The port number on which ftserve is listening.')
    command_group = parser.add_mutually_exclusive_group(required=True)
    command_group.add_argument('-l', '--list', action='store_const', dest='command', const='LIST', help='List files in the server directory.')
    command_group.add_argument('-g', '--get', action='store', dest='filename', help='Get the specified file from ftserve.', metavar='FILENAME')
    command_group.add_argument('-c', '--cd', action='store', dest='dirname', help='Change directories on ftserve.', metavar='DIRNAME')
    parser.add_argument('data_port', help='The client port to use for incoming data transfers.')
    args = parser.parse_args()

    # If a filename was specified but no command,
    # it means the user specified the -g option
    if args.command is None and args.filename is not None:
        args.command = 'GET'
    # If a dirname was specified but no command,
    # it means the user specified the -c option
    elif args.command is None and args.dirname is not None:
        args.command = 'CD'
    return args

def is_int(val):
    """
    Determines whether the specified string represents an integer.

    Returns True if it can be represented as an integer, or False otherwise.
    """
    try:
        int(val)
        return True
    except ValueError:
        return False

class Socket:
    """
    Creates a TCP socket using the socket library.

    This class provides a number of methods to interact with the socket.
    The send and receive functions return a bool value indicating whether
    the socket is still open.
    """

    def __init__(self, sock=None):
        """
        Initializes the object as a TCP socket.
        """
        if sock is None:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        else:
            self.sock = sock

    def connect(self, host, port):
        """
        Attempts to connect to the specified host and port.
        """
        self.sock.connect((host, int(port)))

    def close(self):
        """
        Explicitly closes the socket.
        """
        self.sock.shutdown(socket.SHUT_RDWR)
        self.sock.close()

    def listen(self, port):
        """
        Listens for incoming connections on the specified port.

        Based on code in https://docs.python.org/3/library/socket.html
        """
        #Attempt to bind on the specified port using IPv4 or IPv6
        for family, socktype, protocol, canonname, sockaddr in socket.getaddrinfo(None, port, socket.AF_UNSPEC, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
            # Attempt to open a socket based on localhost address info
            try:
                self.sock = socket.socket(family, socktype, protocol)
            except OSError as ex:
                self.sock = None
                continue

            # Configure the socket to reuse ports
            try:
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            except OSError as ex:
                self.sock = None
                continue

            # Attempt to bind the sockaddr and listen
            try:
                self.sock.bind(sockaddr)
            except OSError as ex:
                self.sock.close()
                s = None
                continue
            # successfully bound if we reach here
            break

        if self.sock is None:
            raise Exception('bind: No valid address found')

        # Try to listen on the socket
        try:
            self.sock.listen(1) # only accept 1 connection
        except OSError as ex:
            raise Exception('listen: ' + ex)

    def accept(self):
        """
        Accepts an incoming connection.

        Returns a Socket object for the new connection.
        """
        newSock, self.remote_address = self.sock.accept()
        return Socket(newSock)

    def send(self, data):
        """
        Sends all of the data unless the socket is closed before it finishes.
        Returns whether the socket is still open.
        """
        bytes = 0
        while (len(data) > 0):
            bytes = self.sock.send(data)
            if (bytes == 0):
                # Socket was closed, return false
                return False

            data = data[bytes:]

        # Return true if socket is still open
        return True

    def recv(self):
        """
        Receives up to 500 bytes of data.
        Returns a tuple including whether the socket is still open,
        and the received data (or an empty string if none).
        """
        buf = self.sock.recv(500)

        if len(buf) == 0:
            return False, buf
        else:
            return True, buf

    def recv_all(self, length):
        """
        Receives the specified number of bytes of data.
        Returns a tuple including whether the socket is still open,
        and the received data (or an empty string if none).
        """
        received = 0
        buf = []
        while received < length:
            data = self.sock.recv(500)
            # Return False and anything that was received if the socket was closed
            if len(data) == 0:
                return False, ''.join(buf)
            # Otherwise append to data already received
            received += len(data)
            buf.append(data)

        return True, ''.join(buf)

# This just makes sure the code doesn't run when imported as a module
if __name__ == '__main__':
    main()
