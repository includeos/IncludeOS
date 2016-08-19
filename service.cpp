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
#include <timer>
#include "ircd.hpp"

IrcServer* ircd;

void print_stats(uint32_t)
{
  static std::vector<int> M;
  static int last = 0;
  // only keep 5 measurements
  if (M.size() > 4) M.erase(M.begin());
  
  int diff = ircd->get_counter(STAT_TOTAL_CONNS) - last;
  last = ircd->get_counter(STAT_TOTAL_CONNS);
  // 2 seconds between measurements
  M.push_back(diff / 2);
  
  double cps = 0.0;
  for (int C : M) cps += C;
  cps /= M.size();
  
  printf("Conns/sec %f  Local clients %d  ",  
      cps, ircd->get_counter(STAT_LOCAL_USERS));
  extern int _get_timer_stats();
  printf("Timers/sec: %u\n", _get_timer_stats() / 2);
  
  StackSampler::print();
  //print_heap_info();
}

void Service::start(const std::string& args)
{
  if (!args.empty())
    printf("Command line is \"%s\"\n", args.c_str());
  
  static std::unique_ptr< net::Inet4<VirtioNet> > inet;
  inet = net::new_ipv4_stack(
      {  10, 0,  0, 42 },  // IP
      { 255,255,255, 0 },  // Netmask
      {  10, 0,  0,  1 }); // Gateway
  
  // IRC default port
  static std::vector<std::string> motd;
  motd.push_back("Welcome to the");
  motd.push_back("IncludeOS IRC server");
  motd.push_back("-§- 4Head -§-");
  
  ircd =
  new IrcServer(*inet, 6667, "irc.includeos.org", "IncludeNet",
  [] () -> const std::vector<std::string>& {
    return motd;
  });
  
  using namespace std::chrono;
  Timers::periodic(2s, 2s, print_stats);
  
  StackSampler::begin();
}

void Service::stop()
{
  printf("hest\n");
}

void Service::ready()
{
  printf("*** IRC SERVICE STARTED ***\n");
}
