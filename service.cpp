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

static const uint16_t    serverID   = 0;
static const std::string servername = "irc.includeos.org";
#include "ircd.hpp"
IrcServer* ircd = nullptr;

extern "C"
void _enable_heap_debugging_verbose(int enabled);
extern "C"
void _enable_heap_debugging_buffer_protection(int enabled);

extern void print_heap_allocations(delegate<bool(void*, size_t)>);

extern "C"
void kernel_sanity_checks();

#include <hw/serial.hpp>
void Service::start()
{
  // add own serial out after service start
  auto& com1 = hw::Serial::port<1>();
  OS::add_stdout(com1.get_print_handler());

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

  ircd =
  new IrcServer(inet, 6667, 7000, 
                serverID, servername, "IncludeNet",
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

Rewritten clients, channels and servers to use perf_array! Hope nothing broke...
)M0TDT3XT";
    return motd;
  });

  ircd->add_remote_server(
    {"irc.other.net", "password123", {46,31,184,184}, 7000});
  // connect to all known remote servers
  ircd->call_remote_servers();

  printf("%s\n", ircd->get_motd().c_str());
  printf("This is server version " IRC_SERVER_VERSION "\n");

  /// LiveUpdate on port 666 ///
  extern void liveupdate_init(net::Inet<net::IP4>&, uint16_t);
  liveupdate_init(inet, 666);
  /// LiveUpdate ///
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

#define PERIOD_SECS    30

void print_stats(int)
{
#ifdef USE_STACK_SAMPLING
  StackSampler::set_mask(true);
#endif

  static std::deque<int> M;
  static int last = 0;
  // only keep 5 measurements
  if (M.size() > 4) M.pop_front();

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
  
  printf("Syns: %u  Conns: %u  Users: %u  RAM: %u bytes Chans: %u\n",
         ircd->get_counter(STAT_TOTAL_CONNS), 
         inet.tcp().active_connections(), 
         ircd->get_counter(STAT_LOCAL_USERS),
         ircd->club(),
         ircd->get_counter(STAT_CHANNELS));
  printf("*** ---------------------- ***\n");
#ifdef USE_STACK_SAMPLING
  // stack sampler results
  ssampler_print(20);
  printf("*** ---------------------- ***\n");
#endif
  // heap statistics
  print_heap_info();
  printf("*** ---------------------- ***\n");
  //printf("%s\n", ScopedProfiler::get_statistics().c_str());
  //ircd->print_stuff();
  /*
  _enable_heap_debugging_buffer_protection(0);
  print_heap_allocations(
  [] (void*, size_t len) -> bool {
    return len >= 0x1000;
  });
  */
  //printf("%s\n", inet.tcp().to_string().c_str());
  kernel_sanity_checks();

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
