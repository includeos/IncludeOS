#!/bin/bash
boot . -b -g
pushd build
~/github/IncludeOS/etc/vmware tcp_perf.grub
popd
