#! /usr/bin/python

import sys
import os

includeos_src = os.environ['INCLUDEOS_SRC']
sys.path.insert(0,includeos_src + "/test")

import vmrunner
vmrunner.vms[0].make().boot(30)
