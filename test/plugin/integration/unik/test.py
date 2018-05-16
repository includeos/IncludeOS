#! /usr/bin/python

import sys
import os

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

from vmrunner import vmrunner

# TODO: Implement a mockup of the Unik registration protocol on 10.0.0.56

vmrunner.vms[0].cmake().boot(60).clean()
