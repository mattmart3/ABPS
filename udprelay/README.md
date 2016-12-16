#UDP Relay

``udprelay.py`` is a simple python application that relays UDP traffic between end nodes.

Its activation is quite simple. From the proxy server that you elected as
the RTP relay do the following:

	python3 udprelay.py 5001:5002

Start the ``udprelay`` process which listens on two UDP ports.
Indeed you can specify any ports you like but keep in mind the first is
where the Mobile Node attaches to and the second is where the
fixed Correspondent Node attaches to.

