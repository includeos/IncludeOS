#! /usr/bin/python

import sys
sys.path.insert(0,"..")

import vmrunner
vmrunner.vms[0].make().boot(30)
