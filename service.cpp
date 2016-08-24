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

#include <ctime>
std::string now()
{
  auto  tnow = time(0);
  auto* curtime = localtime(&tnow);

  char buffer[48];
  int len = strftime(buffer, sizeof(buffer), 
            "%c", curtime);
  return std::string(buffer, len);
}

#include "ircd.hpp"
IrcServer* ircd;


#define PERIOD_SECS    2

void print_stats(uint32_t)
{
  static std::vector<int> M;
  static int last = 0;
  // only keep 5 measurements
  if (M.size() > 4) M.erase(M.begin());
  
  int diff = ircd->get_counter(STAT_TOTAL_CONNS) - last;
  last = ircd->get_counter(STAT_TOTAL_CONNS);
  // @PERIOD_SECS between measurements
  M.push_back(diff / PERIOD_SECS);
  
  double cps = 0.0;
  for (int C : M) cps += C;
  cps /= M.size();
  
  printf("[%s] Conns/sec %f  Local clients %d  ",  
      now().c_str(), cps, ircd->get_counter(STAT_LOCAL_USERS));
  extern int _get_timer_stats();
  printf("Timers/sec: %f\n", _get_timer_stats() / (float)PERIOD_SECS);
  
  StackSampler::print();
  //print_heap_info();
}

extern "C" void _print_elf_symbols();

void Service::start(const std::string& args)
{
  if (!args.empty())
    printf("Command line is \"%s\"\n", args.c_str());
  
  auto& inet = net::Inet4::stack<0>();
  inet.negotiate_dhcp(5.0,
  [&inet] (bool timeout) {
    if (timeout) {
      inet.network_config(
          {  10, 0,  0, 42 },  // IP
          { 255,255,255, 0 },  // Netmask
          {  10, 0,  0,  1 },  // Gateway
          {  10, 0,  0,  1 }); // DNS
    }
  });
  
  // IRC default port
  static std::vector<std::string> motd;
  motd.push_back("Welcome to the");
  motd.push_back("IncludeOS IRC server");
  motd.push_back("-§- 4Head -§-");
  
  ircd =
  new IrcServer(inet, 6667, "irc.includeos.org", "IncludeNet",
  [] () -> const std::vector<std::string>& {
    return motd;
  });
  
  using namespace std::chrono;
  Timers::periodic(seconds(PERIOD_SECS), seconds(PERIOD_SECS), print_stats);
  
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
