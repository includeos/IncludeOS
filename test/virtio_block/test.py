#! /usr/bin/python
import sys
import subprocess

sys.path.insert(0,"..")
subprocess.call(['./image.sh'])

def cleanup():
  subprocess.call(['./cleanup.sh'])

import vmrunner
vmrunner.on_success = cleanup
vmrunner.on_panic = cleanup
vmrunner.vms[0].boot()
