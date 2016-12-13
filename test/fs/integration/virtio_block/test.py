#! /usr/bin/env python
import sys
import subprocess
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)
subprocess.call(['./image.sh'])

def cleanup():
  subprocess.call(['./cleanup.sh'])

from vmrunner import vmrunner
vm = vmrunner.vms[0]

vm.on_exit(cleanup)
vm.cmake().boot(50).clean()
