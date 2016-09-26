#!/usr/bin/env python3
import sys, socket

MAX_BUFF_SIZE = 32768
LOCALHOST = '127.0.0.1'

def fail(reason):
    sys.stderr.write(reason + '\n')
    sys.exit(1)

if len(sys.argv) != 4:
    fail('Usage: cnproxy.py remote_host remote_port localport')

remote_host = sys.argv[1]
remote_port = sys.argv[2]
localport = sys.argv[3]

try:
    remote_port = int(remote_port)
except:
    fail('Invalid port number: ' + str(remote_port))
try:
    localport = int(localport)
except:
    fail('Invalid port number: ' + str(localport))

try:
    se = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
except:
    fail('Failed to create external socket')

try:
    sl = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
except:
    fail('Failed to create local socket')

se.sendto('break fw'.encode(), (remote_host, remote_port))

while True:
    data, addr = se.recvfrom(MAX_BUFF_SIZE)
    se.sendto(data, (LOCALHOST, localport))

