# ABPS

*Always Best Packet Switching (ABPS)* is a software architecture which allows
seamless mobility for Linux and Android devices while using VoIP and real-time multimedia applications.
This repository is a collection of software tools, applications and kernel
patches that enables the ABPS features.

This collection includes:
- TED patches: linux kernel patches that enables *Transmission Error Detector* notification system.
- TED proxy: a proxy application to facilitate vertical handover between WiFi and cellular network through the use of TED notifications
- UDP Relay: a simple python application that relays UDP traffic between end nodes
- Correspondent Node (CN) Tools: simple software tools to enable a receiving counterpart

You can find more documentation for each component in their corresponding
 subdirectory or in the `doc` folder.  
TED patches and TED proxy both reside in `tedproxy` folder.

See the following for more information on ABPS.
* Articles:
	- *The “Always Best Packet Switching” architecture for SIP-based mobile multimedia services*  
	   http://www.sciencedirect.com/science/article/pii/S0164121211001506 (Restricted Access)
	- *Walking with the oracle: Efficient use of mobile networks through location-awareness*   
	   http://ieeexplore.ieee.org/document/6402830/ (Restricted Access)

* Past Thesis:
	- *Matteo Martelli's master thesis* is held in the doc folder of this repository   
      [doc/matteo_martelli_master_thesis/matteo_martelli_dissertation.pdf](doc/matteo_martelli_master_thesis/matteo_martelli_dissertation.pdf)
	- *Alessandro Mengoli's bachelor thesis* (Italian language)  
      http://amslaurea.unibo.it/8981/1/mengoli_alessandro_tesi.pdf
	- *Gabriele Mengoli's bachelor thesis* (Italian language)  
      http://amslaurea.unibo.it/8982/1/dibernardo_gabriele_tesi.pdf 
	- *Luca Milioli's master thesis* (Italian language)  
      http://amslaurea.unibo.it/8368/1/milioli_luca_tesi.pdf (Restricted Access)

If you want to contribute feel free to fork this repository and work on new development branches.  
You may also send me pull requests for new features.

##Run ABPS components
After you read the documentation and you installed and configured ABPS components, you can stream some UDP
traffic from the multi-homed mobile device to the CN passing through the relay.  
For instance
let us consider an RTP camera streamer installed on the mobile device that
sends RTP packets to the CN. On the mobile device you must first run ``tedproxy``:

	./path_to_tedproxy/tedproxy -b -i t:wlan1 -i rmnet0 5006 130.136.4.138 5001

then activate the relay application on the remote proxy:

	python3 /path_to_udprelay/udprelay.py 5001:5002

Now start the CN tools with the ``launcher.sh`` script:

	./path_to_cntools/launcher.sh 130.136.4.138 5002 5006 outputvideo.ogg

It will send an initialization packet to the relay and then it will wait for user
confirmation before starting ``ffmpeg``. Before confirming, start the
stream from the mobile device. Be sure the RTP streamer application sends
packets to 127.0.0.1 and UDP port 5006 which is the port ``tedproxy`` is
listening to. I used a proprietary camera streamer application
(https://play.google.com/store/apps/details?id=com.miv.rtpcamera).
However any similar application should work well.
