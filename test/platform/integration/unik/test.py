#! /usr/bin/python

import sys
import os

includeos_src = os.environ['INCLUDEOS_SRC']
sys.path.insert(0,includeos_src + "/test")

import vmrunner

# TODO: Implement a mockup of the Unik registration protocol on 10.0.0.42

vmrunner.vms[0].make().boot(60)
