#!/usr/bin/python
import subprocess
import os

args = ['sudo', '-E', os.environ['INCLUDEOS_PREFIX'] + '/bin/lxp-run']
res = subprocess.check_output(args)
text = res.decode('utf-8')
#print text
assert "TCP demo started" in text
assert "Server received" in text
print ">>> Linux Userspace TCP test success!"
