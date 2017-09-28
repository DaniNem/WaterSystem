#!/usr/bin/env python
import socket
import threading
import time

def keep_alive(s):
    s.send("KA".encode())
    threading.Timer(10, keep_alive, [s]).start()

TCP_IP = '192.168.1.6'
TCP_PORT = 2000
BUFFER_SIZE = 1024
#s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#s.connect((TCP_IP, TCP_PORT))
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
#s.send('Test'.encode());
#s.close()
#keep_alive(s)
while 1:
    #s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    MESSAGE = input("Enter: ")
    #s.connect((TCP_IP, TCP_PORT))
    s.send(MESSAGE.encode())
    if (MESSAGE == "Echo"):
        a = s.recv(10)
        print(a)
    #s.close()

