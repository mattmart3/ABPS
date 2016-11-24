#!/usr/bin/env python3

# Simple script that implements an UDP relay channel
# Assumes that both sides are sending and receiving from the same port number
# Anything that comes in on left side will be forwarded to right side (once right side origin is known)
# Anything that comes in on right side will be forwarded to left side (once left side origin is known)

# Inspired by https://github.com/EtiennePerot/misc-scripts/blob/master/udp-relay.py

MAX_BUFF_SIZE = 32768

import sys, socket, select, bitstring, time

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

class RtpPacket:
	def __init__(self, data):
		# turn the whole string-of-bytes packet into a string of bits.  Very unefficient, but hey, this is only for demoing.
		bt=bitstring.BitArray(bytes=data)
		self.version=bt[0:2].uint # version
		self.p=bt[3] # P
		self.x=bt[4] # X
		self.cc=bt[4:8].uint # CC
		self.m=bt[9] # M
		self.pt=bt[9:16].uint # PT
		self.sn=bt[16:32].uint # sequence number
		self.timestamp=bt[32:64].uint # timestamp
		self.ssrc=bt[64:96].uint # ssrc identifier
		# The header format can be found from:
		# https://en.wikipedia.org/wiki/Real-time_Transport_Protocol
	
	def getSn(self):
		return self.sn

	def getTimestamp(self):
		return self.timestamp

	def getSsrc(self):
		return self.ssrc

leftSources = None
rightSource = None

leftLastRtpSNs = {}
#TODO Handle send to right duplicate suppress
print('All set.\n')
while True:
	ready_socks,_,_ = select.select([sl, sr], [], []) 
	for sock in ready_socks:
		data, addr = sock.recvfrom(MAX_BUFF_SIZE)
		if sock.fileno() == sl.fileno():
			rtp = RtpPacket(data)
			ssrc = rtp.getSsrc()
			timestamp = rtp.getTimestamp()
			sn = rtp.getSn()
			lastSN = leftLastRtpSNs.get(ssrc, None)
			sentto = "none"

			#If this is the first packet for the ssrc source id 
			if (lastSN == None):
				lastSN = leftLastRtpSNs[ssrc] = sn

			if leftSources is None:
				leftSources = []

			if addr not in leftSources:
				leftSources.append(addr);

			if rightSource is not None:
				if sn > lastSN:
					sr.sendto(data, rightSource)
					leftLastRtpSNs[ssrc] = sn
					sentto = rightSource

			print("lxrecvfrom: ", addr , " sn: "+ str(sn) + " ssrc: " + str(ssrc) + " rtpts: " + str(timestamp) + " rxsentto: " , sentto , " ts: " + str(time.time()))
		else :
			if sock.fileno() == sr.fileno():
				print("Received on right socket from " , addr)
				rightSource = addr;
				if leftSources is not None:
					for addr in leftSources:
						print("Forwarding right to left ", addr)
						sl.sendto(data, addr) 
