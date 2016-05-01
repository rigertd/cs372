import socket
import errno
import os

class Socket:
    """
    Custom wrapper class for the socket library.
    """

    def __init__(self, sock=None):
        if sock is None:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        else:
            self.sock = sock
    
    def connect(self, host, port):
        self.sock.connect((host, int(port)))
    
    def send(self, data):
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
        buf = self.sock.recv(500)
            
        if len(buf) == 0:
            return False, buf
        else:
            return True, buf
    
    
