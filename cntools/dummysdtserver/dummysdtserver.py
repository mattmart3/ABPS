#!/usr/bin/env python3

import http.server
import socketserver

PORT = 8080

Handler = http.server.SimpleHTTPRequestHandler

httpd = socketserver.TCPServer(("", PORT), Handler)

httpd.allow_reuse_address = True


print("serving at port", PORT)
httpd.serve_forever()
