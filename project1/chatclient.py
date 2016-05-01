#!/bin/python
"""
A simple TCP-based chat client program
"""

import readline
import sys
import select
import time
from Socket import Socket


def main():
    if len(sys.argv) != 3:
        print "invalid syntax: ", sys.argv[0], "server-hostname port#"
        sys.exit(1)
    
    # Create a socket
    s = Socket()
    
    # Attempt to connect to server
    try:
        s.connect(sys.argv[1], sys.argv[2])
    except Exception as e:
        print "connect: ", e
        sys.exit(1)
    
    # Check stdin and socket for input
    potential_readers = [sys.stdin, s.sock]
    
    # Input buffer
    input = ''
    
    # Flag to indicate when socket is closed
    is_open = True
    
    # Loop until input is \quit
    while input != r'\quit':
        # Send any input and clear the buffer
        if len(input) > 0:
            s.send(input)
            input = ''
        
        # Check for input on stdin
        readable, writable, exceptional = \
				select.select(potential_readers, [], [], 0.01)
        
        # If no input on stdin, get any data available on socket
        for file in readable:
            if file is sys.stdin:
                input = file.readline().rstrip()
                # break so we don't receive any socket data until
                # the client finishes typing
                break
            else:
                is_open, message = s.recv()
                if len(message) > 0:
                    print message
        
        if not is_open:
            print "socket closed by remote host"
            break

# This just makes sure the code doesn't run when imported as a module
if __name__ == '__main__':
    main()
