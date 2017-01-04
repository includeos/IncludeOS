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
#define USE_STACK_SAMPLING

#include "ircd.hpp"
static IrcServer* ircd;

// prevent default serial out
void default_stdout_handlers() {}
#include <hw/serial.hpp>

void Service::start(const std::string& args)
{
  // add own serial out after service start
  auto& com1 = hw::Serial::port<1>();
  OS::add_stdout(com1.get_print_handler());

  std::string servername = "irc.includeos.org";
  /// extract custom servername from args, if there is one
  size_t idx = args.find(" ");
  if (idx != args.npos) {
    std::string param = std::string(args.begin() + idx + 1, args.end());
    if (param.size()) servername = param;
  }

  // show that we are starting :)
  printf("*** %s starting up...\n", servername.c_str());
  //_enable_heap_debugging_verbose(1);

  // default configuration (with DHCP)
  auto& inet = net::Inet4::ifconfig<>(10);
  inet.network_config(
      {  10, 0,  0, 42 },  // IP
      { 255,255,255, 0 },  // Netmask
      {  10, 0,  0,  1 },  // Gateway
      {  10, 0,  0,  1 }); // DNS

  // IRC default port
  ircd =
  new IrcServer(inet, 6667, servername, "IncludeNet",
  [] () -> const std::string& {
    static const std::string motd = R"M0TDT3XT(
              .-') _                               _ .-') _     ('-.                       .-')
             ( OO ) )                             ( (  OO) )  _(  OO)                     ( OO ).
  ,-.-') ,--./ ,--,'  .-----. ,--.     ,--. ,--.   \     .'_ (,------.       .-'),-----. (_)---\_)
  |  |OO)|   \ |  |\ '  .--./ |  |.-') |  | |  |   ,`'--..._) |  .---'      ( OO'  .-.  '/    _ |
  |  |  \|    \|  | )|  |('-. |  | OO )|  | | .-') |  |  \  ' |  |          /   |  | |  |\  :` `.
  |  |(_/|  .     |//_) |OO  )|  |`-' ||  |_|( OO )|  |   ' |(|  '--.       \_) |  |\|  | '..`''.)
 ,|  |_.'|  |\    | ||  |`-'|(|  '---.'|  | | `-' /|  |   / : |  .--'         \ |  | |  |.-._)   \
(_|  |   |  | \   |(_'  '--'\ |      |('  '-'(_.-' |  '--'  / |  `---.         `'  '-'  '\       /
  `--'   `--'  `--'   `-----' `------'  `-----'    `-------'  `------'           `-----'  `-----'
)M0TDT3XT";
    return motd;
  });

  printf("%s\n", ircd->get_motd().c_str());
}

#include <ctime>
std::string now()
{
  auto  tnow = time(0);
  auto* curtime = localtime(&tnow);

  char buff[48];
  int len = strftime(buff, sizeof(buff), "%c", curtime);
  return std::string(buff, len);
}

#ifdef USE_STACK_SAMPLING
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
#endif

void print_heap_info()
{
  static intptr_t last = 0;
  // show information on heap status, to discover leaks etc.
  extern uintptr_t heap_begin;
  extern uintptr_t heap_end;
  intptr_t heap_size = heap_end - heap_begin;
  last = heap_size - last;
  printf("Heap begin  %#x  size %u Kb\n",     heap_begin, heap_size / 1024);
  printf("Heap end    %#x  diff %u (%d Kb)\n", heap_end,  last, last / 1024);
  last = (int32_t) heap_size;
}

#define PERIOD_SECS    2

void print_stats(int)
{
#ifdef USE_STACK_SAMPLING
  StackSampler::set_mask(true);
#endif

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

  printf("[%s] Conns/sec %.1f  Heap %.1f kb\n",
      now().c_str(), cps, OS::heap_usage() / 1024.0);
  // client and channel stats
  auto& inet = net::Inet4::stack<0>();
  
  printf("Conns: %u  Clis: %u  Alive: %u  Chans: %u\n",
         ircd->get_counter(STAT_TOTAL_CONNS), ircd->get_counter(STAT_LOCAL_USERS),
         inet.tcp().active_connections(), ircd->get_counter(STAT_CHANNELS));
  printf("*** ---------------------- ***\n");
#ifdef USE_STACK_SAMPLING
  // stack sampler results
  ssampler_print(20);
  printf("*** ---------------------- ***\n");
#endif
  // heap statistics
  print_heap_info();
  printf("*** ---------------------- ***\n");
  printf("%s\n", ScopedProfiler::get_statistics().c_str());
  printf("*** ---------------------- ***\n");

#ifdef USE_STACK_SAMPLING
  StackSampler::set_mask(false);
#endif
}

void Service::ready()
{
  printf("*** IRC SERVICE STARTED ***\n");
#ifdef USE_STACK_SAMPLING
  StackSampler::begin();
  StackSampler::set_mode(StackSampler::MODE_CALLER);
#endif

  using namespace std::chrono;
  Timers::periodic(seconds(1), seconds(PERIOD_SECS), print_stats);
}
