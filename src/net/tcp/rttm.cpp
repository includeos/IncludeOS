// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os> // uptime
#include <net/tcp/rttm.hpp>

using namespace net::tcp;

const RTTM::duration_t RTTM::CLOCK_G;

void RTTM::start() {
  t = OS::uptime();
  active = true;
}

void RTTM::stop(bool first) {
  assert(active);
  active = false;
  // round trip time (RTT)
  auto rtt = OS::uptime() - t;
  debug2("<RTTM::stop> RTT: %ums\n",
    (uint32_t)(rtt * 1000));
  if(!first)
    sub_rtt_measurement(rtt);
  else {
    first_rtt_measurement(rtt);
  }
}
