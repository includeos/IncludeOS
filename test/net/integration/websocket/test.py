#! /usr/bin/env python

from __future__ import print_function
from future import standard_library
standard_library.install_aliases()
from builtins import str
import os
import sys
import subprocess
import _thread
import time
from ws4py.client.threadedclient import WebSocketClient

from vmrunner import vmrunner

class DummyClient(WebSocketClient):
    def opened(self):
        self.count = 0
        print("<test.py> Opened")
        time.sleep(1)

    def closed(self, code, reason=None):
        print("<test.py> Closed down", code, reason)

    def handshake_ok(self):
        print("<test.py> Handshake ok")
        self._th.start()

    def close(self, code=1000, reason=''):
        print("close is called, code: {0}, reason: {1}".format(code, reason))

        if not self.client_terminated:
            self.client_terminated = True

            self._write(self.stream.close(code=code, reason=reason).single(mask=True))


    def received_message(self, m):
        #print "<test.py> received message"
        self.count += 1
        if self.count >= 1000:
            print("<test.py> received ", self.count, "messages. Closing.")
            self.close(reason='Bye bye')

def startBenchmark(line):
    print("<test.py> Starting WS benchmark")
    try:
        ws = DummyClient('ws://10.0.0.54:8000/', protocols=['http-only', 'chat'])
        print("<test.py> WS-client connecting")
        ws.connect()
        print("<test.py> WS-client connected, doing run_forever")
        ws.run_forever()
        print("<test.py> Finished running forever")
    except KeyboardInterrupt:
        ws.close()
    return True


def start_ws_thread(line):
    # NOTE: The websocket client is threaded, but it doesn't start a thread until
    # the handshake is complete, which assumes everything works on IncludeOS' side.
    # If it doesn't, control is never returned back to vmrunner and it all hangs.
    print("<test.py> Starting ws client thread")
    _thread.start_new_thread(startBenchmark, (line,))
    print("<test.py> Thread started, returning to vmrunner")

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event for testing server
vm.on_output("Listening on port 8000", start_ws_thread)

if len(sys.argv) > 1:
    vm.boot(image_name=str(sys.argv[1]))
else:
    # Boot the VM, taking a timeout as parameter
    vm.cmake().boot(20,image_name="net_websocket").clean()
