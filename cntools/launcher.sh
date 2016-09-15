#!/usr/bin/bash

if [ $# -lt 4 ] ; then
	echo "Usage: $0 <remote_host> <remote_port> <local_port> <outfile.ogg>"
	exit 1
fi

remote_host=$1
remote_port=$2
local_port=$3
outfile=$4

#kill previous session
pkill -f "python ./cnproxy.py"
pkill -f dummysdtserver.py

pid=$(netstat -nlpat 2>/dev/null | tail -n +3 | awk '/:8080/{print $4,$NF}' | awk '{print $2}' | cut -d'/' -f1)
reg='^[0-9]+$'
echo $pid
if [[ $pid =~ $reg ]] ; then 
	kill $pid
	echo killed
fi

sleep 2

./cnproxy.py "$remote_host" "$remote_port" "$local_port" &

(cd dummysdtserver ; ./dummysdtserver.py &)

echo "Ready to write file?"
select yn in "y" "n"; do
	case $yn in
		y ) ffmpeg -i "http://127.0.0.1:8080/camera.sdp" "$outfile"; break;;
		n ) exit;;
	esac
done
