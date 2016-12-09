#! /usr/bin/env python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src + "/test")

import vmrunner

HOST = ''
PORT = 9000

import BaseHTTPServer

DO_SERVE = True
class RequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(s):
        s.send_response(200)
        s.send_header("Content-type", "text/plain")
        s.end_headers()
        s.wfile.write("%s" % s.path)


def Client_test(trigger_line):
    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class((HOST, PORT), RequestHandler)
    global DO_SERVE
    while(DO_SERVE):
        httpd.handle_request()
        DO_SERVE = False

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event-handler
vm.on_output("Testing HTTP Client", Client_test)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(20).clean()
