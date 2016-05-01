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
        print "usage: ", sys.argv[0], "server_hostname port"
        sys.exit(1)
    
    # Prompt user for handle
    handle = get_handle()
    
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
                print handle, ">"
                # break so we don't receive any socket data until
                # the client finishes typing
                break
            else:
                is_open, message = s.recv()
                if len(message) > 0:
                    print "\n", message, handle, ">"
        
        if not is_open:
            print "socket closed by remote host"
            break

def get_handle():
    '''
    Gets a handle between 1 and 10 characters from stdin.
    Reprompts the user until a valid handle is entered.
    '''
    handle = ''
    while len(handle) == 0 or len(handle) > 10:
        sys.stdout.write("Enter a handle up to 10 characters: ")
        handle = sys.stdin.readline().rstrip()
        
        if len(handle) == 0:
            print "Your handle must be at least 1 character."
        elif len(handle) > 10:
            print "Your handle cannot be longer than 10 characters."
    
    return handle

# This just makes sure the code doesn't run when imported as a module
if __name__ == '__main__':
    main()
