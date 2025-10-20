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
#include <util/units.hpp>
#include <service>
#include <microLB>
#include <net/interfaces>
#include <statman>
#include <timers>
#include <profile>

using namespace util;

microLB::Balancer* balancer = nullptr;

void print_nic_stats() {
  printf("eth0.sendq_max: %zu, eth0.sendq_now: %zu "
                      "eth0.stat_rx_total_packets: %zu, eth0.stat_tx_total_packets: %zu, "
                      "eth0.stat_rx_total_bytes: %zu, eth0.stat_tx_total_bytes: %zu, "
         "eth0.sendq_dropped: %zu, eth0.rx_refill_dropped: %zu \n",
         Statman::get().get_by_name("eth0.sendq_max").get_uint64(),
         Statman::get().get_by_name("eth0.sendq_now").get_uint64(),
         Statman::get().get_by_name("eth0.stat_rx_total_packets").get_uint64(),
         Statman::get().get_by_name("eth0.stat_tx_total_packets").get_uint64(),
         Statman::get().get_by_name("eth0.stat_rx_total_bytes").get_uint64(),
         Statman::get().get_by_name("eth0.stat_tx_total_bytes").get_uint64(),
         Statman::get().get_by_name("eth0.sendq_dropped").get_uint64(),
         Statman::get().get_by_name("eth0.rx_refill_dropped").get_uint64()
    );

  printf("eth1.sendq_max: %zu, eth1.sendq_now: %zu "
                      "eth1.stat_rx_total_packets: %zu, eth1.stat_tx_total_packets: %zu, "
                      "eth1.stat_rx_total_bytes: %zu, eth1.stat_tx_total_bytes: %zu, "
         "eth1.sendq_dropped: %zu, eth1.rx_refill_dropped: %zu \n",
         Statman::get().get_by_name("eth1.sendq_max").get_uint64(),
         Statman::get().get_by_name("eth1.sendq_now").get_uint64(),
         Statman::get().get_by_name("eth1.stat_rx_total_packets").get_uint64(),
         Statman::get().get_by_name("eth1.stat_tx_total_packets").get_uint64(),
         Statman::get().get_by_name("eth1.stat_rx_total_bytes").get_uint64(),
         Statman::get().get_by_name("eth1.stat_tx_total_bytes").get_uint64(),
         Statman::get().get_by_name("eth1.sendq_dropped").get_uint64(),
         Statman::get().get_by_name("eth1.rx_refill_dropped").get_uint64()
    );
  printf("\n\n");
}

void print_mempool_stats() {
    auto& inet1 = net::Interfaces::get(0);
    auto& inet2 = net::Interfaces::get(1);;
    printf("\n\n<Timer>Mem used: %s\n", util::Byte_r(os::total_memuse()).to_string().c_str());
    auto pool1 = inet1.tcp().mempool();
    auto pool2 = inet2.tcp().mempool();

    // Hack to get the implementation details (e.g. the detail::pool ptr) for some stats
    auto res1 = pool1.get_resource();
    auto res2 = pool2.get_resource();

    auto pool_ptr1 = res1->pool();
    auto pool_ptr2 = res2->pool();

    res1.reset();
    res2.reset();

    printf("\n*** TCP0 ***\n%s\n pool: %zu / %zu allocs: %zu resources: %zu (used: %zu free: %zu)\n\n",
           inet1.tcp().to_string().c_str(), pool1.allocated(), pool1.total_capacity(), pool1.alloc_count(),
           pool1.resource_count(), pool_ptr1->used_resources(),
           pool_ptr1->free_resources());
    printf("*** TCP1 ***\n%s\npool: %zu / %zu allocs: %zu resources: %zu (used: %zu free: %zu)\n",
           inet2.tcp().to_string().c_str(), pool2.allocated(), pool2.total_capacity(), pool2.alloc_count(),
           pool2.resource_count(), pool_ptr2->used_resources(),
           pool_ptr2->free_resources());
}

void print_lb_stats() {
  FILLINE('-');
  CENTER("LB-Stats");
  auto& nodes = balancer->nodes;
  printf("Wait queue: %i nodes: %zu tot_sess: %zi open_sess: %i timeout_sess: %i pool_size: %i \n",
         balancer->wait_queue(), nodes.size(), nodes.total_sessions(), nodes.open_sessions(), nodes.timed_out_sessions(), nodes.pool_size());
  printf("\n\n");
}

void Service::start()
{
  balancer = microLB::Balancer::from_config();
  printf("MicroLB ready for test\n");
  auto& inet1 = net::Interfaces::get(0);
  inet1.tcp().set_MSL(std::chrono::seconds(2));

  // Increasing TCP buffer size may increase throughput
  //inet1.tcp().set_total_bufsize(256_MiB);
  //auto& inet2 = net::Interfaces::get(1);;
  //inet2.tcp().set_total_bufsize(256_MiB);

  Timers::oneshot(std::chrono::seconds(5),
  [] (int) {
    printf("TCP MSL ended (4 seconds)\n");
  });
  //StackSampler::begin();

  Timers::periodic(2s, 5s, [](auto) {
      //StackSampler::print(10);
      print_nic_stats();
      print_mempool_stats();
      print_lb_stats();
    });
}
