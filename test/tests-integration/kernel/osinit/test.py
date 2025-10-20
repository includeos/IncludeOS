#!/usr/bin/env python3

from vmrunner import vmrunner
vm = vmrunner.vms[0]


def done(line):
    if "Healthy" in line:
        vm.exit(0, "OS initialization test succeeded")
    elif "failed" in line:
        status = line.split(":")[1].strip()
        vm.exit(1, status)
    else:
        vm.exit(2, "OS initialization test failed")

vm.on_output("Diagnose complete", done)


vm.boot(20, image_name='osinit.elf.bin')
