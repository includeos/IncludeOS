#! /usr/bin/env python

import os
import sys
import subprocess
import thread

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner
from ws4py.client.threadedclient import WebSocketClient

class DummyClient(WebSocketClient):
    def opened(self):
        self.count = 0
        print "Opened"

    def closed(self, code, reason=None):
        print "Closed down", code, reason

    def received_message(self, m):
        self.count += 1
        if self.count >= 1000:
            self.close(reason='Bye bye')

def startBenchmark(line):
    try:
        ws = DummyClient('ws://10.0.0.42:8000/', protocols=['http-only', 'chat'])
        ws.connect()
        ws.run_forever()
    except KeyboardInterrupt:
        ws.close()
    return True

# Get an auto-created VM from the vmrunner
vm = vmrunner.vms[0]

# Add custom event for testing server
vm.on_output("Listening on port 8000", startBenchmark)

# Boot the VM, taking a timeout as parameter
vm.cmake().boot(20).clean()
