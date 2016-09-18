# This file is a part of the IncludeOS unikernel - www.includeos.org
#
# Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
# and Alfred Bratterud
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#!/bin/bash

# Make mnt/ if it doesn't exist
mkdir -p mnt

# Mount disk if not mounted
if ! mountpoint -q -- mnt/;
then
  echo ">>> Mounting memdisk.fat"
  sudo mount -o sync,rw memdisk.fat mnt/
fi

# Copy web content to mounted disk
sudo cp -r disk1/. mnt/

# Unmount disk
sudo umount mnt/

# Remove stale object file
rm -f memdisk.o

# Build service
make -j
export CPU=""
${INCLUDEOS_HOME-$HOME/IncludeOS_install}/etc/run.sh `make servicefile`
