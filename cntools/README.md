#CN Tools

CN tools is a set of software tools that are required to enable an RTP receiving end on the Correspondent Node. 
First of all you need
``ffmpeg`` installed on the CN machine. At the time of writing
``ffmpeg`` is
not present on debian distributions since it has been replaced with
``avconv``. I did not investigate further but ``avconv`` did not work
well in my test environment. Thus since my CN was running a debian ``jessie``
distribution I compiled ``ffmpeg`` from sources. Once you obtained
``ffmpeg`` working on your CN machine you can run the ``launcher.sh`` script:

	./launcher.sh 130.136.4.138 5002 5006 outputvideo.ogg

where 130.136.4.138 is the remote host IP address, 5002 the remote UDP port and
5006 the local UDP port.

The script first launches the ``dummystdserver`` HTTP server which
serves an SDP file at the address ``http://127.0.0.1:8080/camera.sdp``.
Then it launches ``cnproxy`` passing to it the host IP address and both
ports as arguments and runs ``ffmpeg`` as last step.

When launched ``cnproxy``
sends an initialization datagram to the remote host then starts forwarding
remote host's responses to the local UDP port, 5006 in this case, which ``ffmpeg``
listens to. Thus ``ffmpeg`` first gets the ``camera.sdp`` file from
the ``dummystdserver`` then receives on a local UDP port, specified in the sdp file,
datagrams forwarded by ``cnproxy`` and eventually writes the output video file.
Be sure the remote UDP port corresponds to the one the destination is listening
to and that the local UDP port corresponds to the one specified in the sdp
file.

