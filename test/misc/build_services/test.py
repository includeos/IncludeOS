#! /usr/bin/env python3

import sys
import os
import subprocess
from multiprocessing import Pool

includeos_src = os.environ.get('INCLUDEOS_SRC',
                               os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__))).split('/test')[0])
sys.path.insert(0,includeos_src)

includeos_prefix = os.environ.get('INCLUDEOS_PREFIX',
                               os.path.realpath(os.path.join(os.getcwd())))
sys.path.insert(0,includeos_prefix)

count = 0
count_build = 0
examples = []
tmpfile="/tmp/build_test"
skip_tests="demo_service"

path_to_examples = os.path.join(includeos_src,'examples')
path_to_starbase = os.path.join(includeos_src,'lib/uplink')

def starbase():
    print(">>>Building starbase")
    for find_starbase in next(os.walk(path_to_starbase))[1]:
        if "starbase" in find_starbase:
            starbase_dir = os.path.join(includeos_src, path_to_starbase, find_starbase)

    run_test(stabase_dir)


def build_service(subdir):
    print(">>>Building service:" + subdir)
    service_dir = os.path.join(includeos_src,'examples', subdir)
    run_test(service_dir)


def run_test(run_dir):
    os.chdir(run_dir)
    subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

    if os.path.exists("prereq.sh"):
        print("Installing Prereq")
        subprocess.call("./prereq.sh")

    run_boot_path=os.path.join(includeos_prefix,'bin/boot')
    clean_build = '-cb'
    dot = '.'
    subprocess.call(['/bin/bash', run_boot_path,clean_build, dot,'&>',tmpfile ])
    print(" [ PASS ]")

for subdir in next(os.walk(path_to_examples))[1]:
    if skip_tests in subdir:
        continue

    examples.append(subdir)
    count = count + 1

p = Pool(count)
p.map(build_service, examples)
starbase
