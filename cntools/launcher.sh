#!/usr/bin/bash

if [ $# -lt 4 ] ; then
	echo "Usage: $0 <remote_host> <remote_port> <local_port> <outfile.mp4>"
	exit 1
fi

remote_host=$1
remote_port=$2
local_port=$3
outfile=$4

cnproxy.py "$remote_host $remote_port $local_port" &

(cd dummysdtserver ; ./dummysdtserver.py &)

echo "Ready to write file?"
select yn in "y" "n"; do
	case $yn in
		y ) ffmpeg -i "http:127.0.0.1:8000/camera.sdp $outifle"; break;;
		n ) exit;;
	esac
done
