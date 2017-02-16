#! /usr/bin/python

import sys
import os
from vmrunner import vmrunner

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])

# TODO: Implement a mockup of the Unik registration protocol on 10.0.0.42

vmrunner.vms[0].cmake().boot(60).clean()
