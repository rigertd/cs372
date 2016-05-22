#!/usr/bin/env python
"""
chatclient: A simple TCP-based chat client program.

Author:     David Rigert
Class:      CS372 Spring 2016
Assignment: Project #1

When this program is run, the user is prompted to enter their handle.
The handle will prepend any messages sent from this client. Once the user
enters a message to send, this program attempts to connect to the chatserv
program. After a connection is established, any messages sent from this program
are displayed on the server and any other connected clients, and any messages
sent by the server and other connected clients are displayed in this program.

    Command-line syntax: ./chatclient <server_hostname> <server_port>

This program takes the following arguments:
    - server_hostname       -- The hostname or IP address of the server
    - server_port           -- The port number that the server is listening on
"""

__author__      = "David Rigert"

import readline
import sys
import select
import time
import re
import os
from Socket import Socket
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
    except Exception as e:
        print "connect: ", e
        sys.exit(1)

    # Send command to server along with data port and filename (if any)
    control_sock.send('{0} {1} {2}'.format(args.command, args.data_port, args.filename))

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
        control_sock.send("ACK")

        # Accept the incoming connection
        try:
            data_sock = data_sock.accept()
        except Exception as ex:
            print(ex)
            exit(1)

        # Display required output in terminal window
        if args.command == 'LIST':
            print('Receiving directory structure from {0}:{1}'.format(args.server_host, args.data_port))
        else:
            print('Receiving "{0}" from {1}:{2}'.format(args.filename, args.server_host, args.data_port))
        # Receive the amount of data specified in the response
        is_open, data = data_sock.recv_all(int(response))

        # Acknowledge the receipt of the data
        control_sock.send('ACK')

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

    else:
        # Error message--print it and exit
        print('{0}:{1} says {2}'.format(args.server_host, args.server_port, response))
        control_sock.close()
        exit(1)

def get_unique_filename(name):
    """
    Returns a filename that is guaranteed to be unique in the specified directory.
    """
    if not os.path.exists(name):
        return name
    else:
        m = re.match(r'(.*\()(\d+)(\))(\..+)?', name, re.IGNORECASE)
        if m:
            name = '{0}{1}{2}{3}'.format(m.groups()[0], int(m.groups()[1]) + 1, m.groups()[2], m.groups()[3])
        else:
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
    parser.add_argument('data_port', help='The client port to use for incoming data transfers.')
    args = parser.parse_args()

    # If a filename was specified but no command,
    # it means the user specified the -g option
    if args.command is None and args.filename is not None:
        args.command = 'GET'
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

# This just makes sure the code doesn't run when imported as a module
if __name__ == '__main__':
    main()
