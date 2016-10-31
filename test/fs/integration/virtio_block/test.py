#! /usr/bin/python
import sys
import subprocess
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src + "/test")
subprocess.call(['./image.sh'])

def cleanup():
  subprocess.call(['./cleanup.sh'])

def success(trigger_line):
  cleanup()

def failure(trigger_line):
  cleanup()

import vmrunner
vmrunner.vms[0].on_success(success)
vmrunner.vms[0].on_panic(failure)
vmrunner.vms[0].make().boot(50)
