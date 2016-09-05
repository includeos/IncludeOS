#! /usr/bin/python
import sys
import subprocess
import os

includeos_src = os.environ['INCLUDEOS_SRC']
sys.path.insert(0,includeos_src + "/test")
subprocess.call(['./image.sh'])

def cleanup():
  subprocess.call(['./cleanup.sh'])

def success():
  cleanup()

def failure():
  cleanup()

import vmrunner
vmrunner.vms[0].on_success(success)
vmrunner.vms[0].on_panic(failure)
vmrunner.vms[0].make().boot(50)
