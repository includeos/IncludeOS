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
#include <timers>

#include <ctime>
std::string now()
{
  auto  tnow = time(0);
  auto* curtime = localtime(&tnow);

  char buff[48];
  int len = strftime(buff, sizeof(buff), "%c", curtime);
  return std::string(buff, len);
}

// print N results to stdout
void ssampler_print(int N)
{
  auto samp = StackSampler::results(N);
  int total = StackSampler::samples_total();
  
  printf("Stack sampling - %d results (%u samples)\n", 
         samp.size(), total);
  for (auto& sa : samp)
  {
    // percentage of total samples
    float perc = sa.samp / (float)total * 100.0f;
    printf("%5.2f%%  %*u: %s\n",
           perc, 8, sa.samp, sa.name.c_str());
  }
}

#include "ircd.hpp"
static IrcServer* ircd;


#define PERIOD_SECS    30

void print_stats(uint32_t)
{
  StackSampler::set_mask(true);
  
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
  
  printf("[%s] Conns/sec %f  Heap %.1f kb\n",  
      now().c_str(), cps, OS::heap_usage() / 1024.f);
  // client and channel stats
  printf("Conns: %u  Clis: %u  Club: %u  Chans: %u\n", 
         ircd->get_counter(STAT_TOTAL_CONNS), ircd->get_counter(STAT_LOCAL_USERS), 
         ircd->club(), ircd->get_counter(STAT_CHANNELS));
  printf("*** ---------------------- ***\n");
  // stack sampler results
  ssampler_print(12);
  printf("*** ---------------------- ***\n");
  // heap statistics
  print_heap_info();
  printf("*** ---------------------- ***\n");
  
  StackSampler::set_mask(false);
}

extern "C" void _print_elf_symbols();

void Service::start(const std::string& args)
{
  if (!args.empty())
    printf("Command line is \"%s\"\n", args.c_str());
  
  auto& inet = net::Inet4::ifconfig<0>(10);
  inet.network_config(
      {  10, 0,  0, 42 },  // IP
      { 255,255,255, 0 },  // Netmask
      {  10, 0,  0,  1 },  // Gateway
      {  10, 0,  0,  1 }); // DNS
  
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
}

void Service::stop()
{
  printf("hest\n");
}

void Service::ready()
{
  printf("*** IRC SERVICE STARTED ***\n");
  StackSampler::begin();
}
