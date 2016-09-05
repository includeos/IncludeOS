#!/usr/bin/python

import sys
import os

includeos_src = os.environ['INCLUDEOS_SRC']
sys.path.insert(0,includeos_src + "/test")

from subprocess import call

import vmrunner

vm = vmrunner.vms[0]

def twosector():
  # Run bigdisk when finished
  vm.on_exit(bigdisk)
  # Make twosector service with sector2 disk
  vm.make(["FILES=twosector.cpp", "DISK=sector2.disk"]).boot()

def bigdisk():
  # Run cleanup when finished
  vm.on_exit(cleanup)
  # Create big.disk
  call(["./bigdisk.sh"], shell=True)
  # Make bigdisk service with big disk
  vm.make(["FILES=bigdisk.cpp", "DISK=big.disk"]).boot()

def cleanup():
  # Clean first
  vm.make(["clean"])

  # Clean second
  vm.make(["FILES=twosector.cpp", "clean"])

  # Clean third
  vm.make(["FILES=bigdisk.cpp", "clean"])
  # Also remove big.disk
  call(["rm", "big.disk"])

# Run twosector when finished
vm.on_exit(twosector)

# Make default (nosector) and boot the VM
vm.make().boot(60)
