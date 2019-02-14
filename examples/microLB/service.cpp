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
#include <profile>
#include <microLB>
#include <net/interfaces>
static void print_stats(int);
#define STATS_PERIOD  5s

#include "../LiveUpdate/liu.hpp"
static void save_state(liu::Storage& store, const liu::buffer_t*)
{

}

static microLB::Balancer* balancer = nullptr;
void Service::start()
{
  balancer = microLB::Balancer::from_config();

  Timers::periodic(1s, STATS_PERIOD, print_stats);
  StackSampler::begin();
  //StackSampler::set_mode(StackSampler::MODE_CURRENT);

  // raw TCP liveupdate server
  auto& inet = net::Interfaces::get(0);
  setup_liveupdate_server(inet, 666, save_state);
}

/// statistics ///
#include <timers>
#include <ctime>
using namespace std::chrono;

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
  static auto last = os::total_memuse();
  // show information on heap status, to discover leaks etc.
  auto heap_usage = os::total_memuse();
  auto diff = heap_usage - last;
  printf("Mem usage %zu Kb  diff %zu (%zu Kb)\n",
        heap_usage / 1024, diff, diff / 1024);
  last = heap_usage;
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
  int  growth = totals - last;  last = totals;

  printf("*** [%s] ***\n", now().c_str());
  printf("Total %ld (%+d) Sess %d Wait %d TO %d - Pool %d C.Att %d Err %d\n",
         totals, growth, nodes.open_sessions(), balancer->wait_queue(),
         nodes.timed_out_sessions(), nodes.pool_size(),
         nodes.pool_connecting(), balancer->connect_throws());

  // node information
  int n = 0;
  for (auto& node : nodes) {
    printf("[%s %s P=%d C=%d]  ", node.address().to_string().c_str(),
        (node.is_active() ? "ONL" : "OFF"),
        node.pool_size(), node.connection_attempts());
    if (++n == 2) { n = 0; printf("\n"); }
  }
  if (n > 0) printf("\n");

  // CPU-usage statistics
  static uint64_t last_total = 0, last_asleep = 0;
  uint64_t tdiff = StackSampler::samples_total() - last_total;
  last_total = StackSampler::samples_total();
  uint64_t adiff = StackSampler::samples_asleep() - last_asleep;
  last_asleep = StackSampler::samples_asleep();

  if (tdiff > 0)
  {
    double asleep = adiff / (double) tdiff;
    static rolling_avg<5, double> asleep_avg;
    asleep_avg.push(asleep);

    printf("CPU usage: %.2f%%  Idle: %.2f%%  Active: %ld Existing: %ld Free: %ld\n",
          (1.0 - asleep) * 100.0, asleep * 100.0,
          Timers::active(), Timers::existing(), Timers::free());
  }
  else {
    printf("CPU usage unavailable due to lack of samples\n");
  }

  // heap statistics
  print_heap_info();
  // stack sampling
  StackSampler::print(3);
}
