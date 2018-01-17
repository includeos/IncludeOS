# This file is a part of the IncludeOS unikernel - www.includeos.org
#
# Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
cmake_minimum_required(VERSION 2.8.9)

# IncludeOS install location
if (NOT DEFINED ENV{INCLUDEOS_PREFIX})
  set(ENV{INCLUDEOS_PREFIX} /usr/local)
endif()

include($ENV{INCLUDEOS_PREFIX}/includeos/pre.service.cmake)
project(protobuf_test_service C CXX)

# Human-readable name of your service
set(SERVICE_NAME "Google's protobuf runtime library test")

# Name of your service binary
set(BINARY "protobuf_test_service")

# Local include paths
set(LOCAL_INCLUDES ".")

# Source files to be linked with OS library parts to form bootable image
set(SOURCES service.cpp person.pb.cc)

# Library to be linked into the service image
set(LIBRARIES libprotobuf.a)

# Include service build script
include($ENV{INCLUDEOS_PREFIX}/includeos/post.service.cmake)
