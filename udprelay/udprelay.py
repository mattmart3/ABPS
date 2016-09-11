#!/usr/bin/env python

# Simple script that implements an UDP relay channel
# Assumes that both sides are sending and receiving from the same port number
# Anything that comes in on left side will be forwarded to right side (once right side origin is known)
# Anything that comes in on right side will be forwarded to left side (once left side origin is known)

# Inspired by https://github.com/EtiennePerot/misc-scripts/blob/master/udp-relay.py

import sys, socket, select

def fail(reason):
	sys.stderr.write(reason + '\n')
	sys.exit(1)

if len(sys.argv) != 2 or len(sys.argv[1].split(':')) != 2:
	fail('Usage: udp-relay.py leftPort:rightPort')

leftPort, rightPort = sys.argv[1].split(':')

try:
	leftPort = int(leftPort)
except:
	fail('Invalid port number: ' + str(leftPort))
try:
	rightPort = int(rightPort)
except:
	fail('Invalid port number: ' + str(rightPort))

try:
	sl = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	sl.bind(('', leftPort))
except:
	fail('Failed to bind on port ' + str(leftPort))

try:
	sr = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	sr.bind(('', rightPort))
except:
	fail('Failed to bind on port ' + str(rightPort))

leftSources = None
rightSource = None
sys.stderr.write('All set.\n')
while True:
	ready_socks,_,_ = select.select([sl, sr], [], []) 
	for sock in ready_socks:
		data, addr = sock.recvfrom(32768)
		if sock.fileno() == sl.fileno():
			print "Received on left socket from " , addr
                        if leftSources is None:
                            leftSources = []
			leftSources.append(addr);
			if rightSource is not None:
				print "Forwarding left to right ", rightSource
				sr.sendto(data, rightSource)
		else :
			if sock.fileno() == sr.fileno():
				print "Received on right socket from " , addr
				rightSource = addr;
				if leftSources is not None:
					for addr in leftSources:
					    print "Forwarding right to left ", addr
					    sl.sendto(data, addr)

