#! /usr/bin/env python
from builtins import str
import sys
import subprocess
import subprocess32
import os

thread_timeout = 50

subprocess32.call(['./image.sh'], timeout=thread_timeout)

def cleanup():
  subprocess.call(['./cleanup.sh'])

from vmrunner import vmrunner
vm = vmrunner.vms[0]

vm.on_exit(cleanup)

if len(sys.argv) > 1:
    vm.boot(thread_timeout,image_name=str(sys.argv[1]))
else:
    vm.cmake().boot(thread_timeout,image_name='fs_virtio_block').clean()
