#!/bin/bash
boot -b . && qemu-img convert -O vmdk build/LiveUpdate.img build/LiveUpdate.vmdk
vmrun start vmx/Other\ 64-bit\ \(2\).vmx
