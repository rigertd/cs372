"""
Socket.py: A wrapper class for a TCP socket.

Author:     David Rigert
Class:      CS372 Spring 2016
Assignment: Project #1
"""

import socket

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


