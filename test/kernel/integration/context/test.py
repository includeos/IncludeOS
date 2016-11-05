#! /usr/bin/python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src + "/test")

import vmrunner

vm = vmrunner.vms[0];
vm.make()
vm.boot(20)
