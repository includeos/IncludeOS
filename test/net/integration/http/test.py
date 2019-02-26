#! /usr/bin/env python

import sys
import os
import thread

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner

HOST = ''
PORT = 9011

import BaseHTTPServer

DO_SERVE = True
class RequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(s):
        s.send_response(200)
        s.send_header("Content-type", "text/plain")
        s.end_headers()
        s.wfile.write("%s" % s.path)


def Client_test():
    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class((HOST, PORT), RequestHandler)
    global DO_SERVE
    while(DO_SERVE):
        httpd.handle_request()
        DO_SERVE = False
    httpd.server_close()

# Start web server in a separate thread
thread.start_new_thread(Client_test, ())


import urllib2
def Server_test(triggerline):
    res = urllib2.urlopen("http://10.0.0.46:8080").read()
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
