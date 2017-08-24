// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <os>
#include <timers>
#include <ctime>
using namespace std::chrono;
#define STATS_PERIOD  5s
static void print_stats(int);

#include "balancer.hpp"
static Balancer* balancer = nullptr;
#define NET_UPLINK    0
#define NET_INCOMING  1
#define NET_OUTGOING  2

void Service::start() {}

void Service::ready()
{
  // incoming
  auto& inc = net::Super_stack::get<net::IP4>(NET_INCOMING);
  // outgoing
  auto& out = net::Super_stack::get<net::IP4>(NET_OUTGOING);

  balancer = new Balancer(inc, 80, out);

  Timers::periodic(1s, STATS_PERIOD, print_stats);
}


static std::string now()
{
  auto  tnow = time(0);
  auto* curtime = localtime(&tnow);

  char buff[48];
  int len = strftime(buff, sizeof(buff), "%c", curtime);
  return std::string(buff, len);
}

static void print_heap_info()
{
  static intptr_t last = 0;
  // show information on heap status, to discover leaks etc.
  auto heap_begin = OS::heap_begin();
  auto heap_end   = OS::heap_end();
  auto heap_usage = OS::heap_usage();
  intptr_t heap_size = heap_end - heap_begin;
  auto diff = heap_size - last;
  printf("Heap size %lu Kb  diff %ld (%ld Kb)  usage  %lu kB\n",
        heap_size / 1024, diff, diff / 1024, heap_usage / 1024);
  last = (int32_t) heap_size;
}

template <int N, typename T>
struct rolling_avg {
  std::deque<T> values;

  void push(T value) {
    if (values.size() >= N) values.pop_front();
    values.push_back(value);
  }
  double avg() const {
    double ps = 0.0;
    if (values.empty()) return ps;
    for (auto v : values) ps += v;
    return ps / values.size();
  }
};

void print_stats(int)
{
  static int64_t last = 0;
  const auto& nodes = balancer->nodes;

  auto totals = nodes.total_sessions();
  auto growth = totals - last;  last = totals;

  printf("*** [%s] ***\n", now().c_str());
  printf("Total %ld Gr %+ld  Sess %d Wait %d  Pool %d Wait %d\n",
         totals, growth, nodes.open_sessions(), balancer->wait_queue(),
         nodes.pool_size(), nodes.pool_connecting());
  // node information
  int n = 0;
  for (auto& node : nodes) {
    printf("[%s %s P=%d C=%d]  ", node.address().to_string().c_str(),
        (node.is_active() ? "ONL" : "OFF"),
        node.pool_size(), node.connection_attempts());
    if (++n == 2) { n = 0; printf("\n"); }
  }
  if (n > 0) printf("\n");

  // heap statistics
  print_heap_info();
}
