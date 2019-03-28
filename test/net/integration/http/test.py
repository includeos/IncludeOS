#! /usr/bin/env python

from future import standard_library
standard_library.install_aliases()
from builtins import str
import sys
import os
import _thread

from vmrunner import vmrunner

HOST = ''
PORT = 9011

import http.server

DO_SERVE = True
class RequestHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(s):
        s.send_response(200)
        s.send_header("Content-type", "text/plain; charset=utf-8",)
        s.end_headers()
        s.wfile.write(s.path.encode("utf-8"))


def Client_test():
    server_class = http.server.HTTPServer
    httpd = server_class((HOST, PORT), RequestHandler)
    global DO_SERVE
    while(DO_SERVE):
        httpd.handle_request()
        DO_SERVE = False
    httpd.server_close()

# Start web server in a separate thread
_thread.start_new_thread(Client_test, ())


import urllib.request, urllib.error, urllib.parse
def Server_test(triggerline):
    res = urllib.request.urlopen("http://10.0.0.46:8080").read()
    assert(res == "Hello")


# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event for testing server
vm.on_output("Listening on port 8080", Server_test)

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.cmake().boot(20,image_name='net_http').clean()
