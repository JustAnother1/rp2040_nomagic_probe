#!/usr/bin/python3
# -*- coding: utf-8 -*-

import socket


TCP_IP = '127.0.0.1'
TCP_PORT = 3333
BUFFER_SIZE = 1024


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))

s.send(b'$m10000000,200#ac')

data = s.recv(BUFFER_SIZE)
print("received data:", data)

s.send(b'$m10000000,200#ac')

data = s.recv(BUFFER_SIZE)
print("received data:", data)

data = s.recv(BUFFER_SIZE)
print("received data:", data)


data = s.recv(BUFFER_SIZE)
print("received data:", data)


s.close()


