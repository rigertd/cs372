from fcntl import fcntl
import socket
import errno
import os

class Socket:
'''
Custom wrapper class for the socket library.
Implements a socket with blocking send and non-blocking receive.
'''
    def __init__(self):
        if sock is None:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            fcntl(self.sock, F_SETFL, os.O_NONBLOCK)
        else:
            self.sock = sock
    
    def connect(self, host, port):
        self.sock.connect((host, port))
    
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
        buffer = []
        while True:
            buf = ''
            try:
                buf = self.sock.recv(500)
            except socket.error as ex:
                if ex.args[0] == errno.EAGAIN or ex.args[0] == errno.EWOULDBLOCK:
                    return True, ''.join(buffer)
                else:
                    raise ex
            
            if len(buf) == 0:
                return False, ''.join(buffer)
            
            buffer += buf
    
    