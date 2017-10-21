#!/usr/bin/env python
import socket
import threading
import _thread
import time

def keep_alive(s):
    s.send("KA".encode())
    threading.Timer(10, keep_alive, [s]).start()
def shit(s):
	while 1:
		print(s.recv(2048))
TCP_IP = '192.168.1.200'
#TCP_IP = '192.168.2.200'

TCP_PORT = 8888
BUFFER_SIZE = 1024
#s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#s.connect((TCP_IP, TCP_PORT))
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
_thread.start_new_thread(shit,(s,))
s.send('Test'.encode());
time.sleep(5)
#s.close()
keep_alive(s)
while 1:
    #s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	MESSAGE = input("Enter: ")
    #s.connect((TCP_IP, TCP_PORT))
	s.send(MESSAGE.encode())
	time.sleep(1)
	#shit(s)
    #s.close()

