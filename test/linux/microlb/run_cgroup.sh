#!/bin/bash
CGR=myGroup
# create cgroup
cgcreate -g memory:/$CGR
# 40 MB memory limit
echo $(( 40 * 1024 * 1024 )) > /sys/fs/cgroup/memory/$CGR/memory.limit_in_bytes
# disable OOM killer
echo 1 > /sys/fs/cgroup/memory/$CGR/memory.oom_control
