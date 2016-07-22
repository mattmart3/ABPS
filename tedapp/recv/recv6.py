#Recv ipv6 version.
#TODO: merge with recv4 (ipv4 version)

import sys, os

import socket

import argparse

import json

import time

#example: python recv6.py fe80::226:b0ff:fed7:7dfa%eth0 50000

parser = argparse.ArgumentParser(description="IPv6 address and port")
parser.add_argument('address')
parser.add_argument('port')
args = parser.parse_args()

address = args.address

port = int(args.port)

receivingSocket = socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
print "Socket primed."

result = socket.getaddrinfo(address, port, socket.AF_INET6, socket.SOCK_DGRAM, 0, 0)

family, socketType, proto, canonName, socketAddress = result[0]
receivingSocket.bind(socketAddress)
print "Socket created."
while 1:
    data, address = receivingSocket.recvfrom(8000)
    print str(data)
    print "-------------"

os.close(logFileDescriptor)
