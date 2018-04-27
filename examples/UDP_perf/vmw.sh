#!/bin/bash
boot . -b -g
pushd build
~/github/IncludeOS/etc/vmware udp_perf.grub
popd
