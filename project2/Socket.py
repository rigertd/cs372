"""
Socket.py: A wrapper class for a TCP socket.

Author:     David Rigert
Class:      CS372 Spring 2016
Assignment: Project #2
"""
__author__ = 'David Rigert'

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

    def recv(self, length):
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
