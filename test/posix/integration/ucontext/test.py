#! /usr/bin/env python
import sys
import os
from vmrunner import vmrunner

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])

vm = vmrunner.vms[0];
vm.cmake().boot(20).clean()
