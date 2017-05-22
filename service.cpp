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

#include <service>
#include <cstdio>
#include <net/inet4>
#include <statman>
#include <profile>
#include <os>

using namespace net;
using namespace net::tcp;

std::unique_ptr<Chunk> blob;
uint32_t  SIZE = 1024*1024*512;
uint64_t  packets_rx{0};
uint64_t  packets_tx{0};
uint64_t  received{0};
uint32_t  winsize{8192};
uint8_t   wscale{5};
bool      timestamps{true};
std::chrono::milliseconds dack{40};

extern "C" void kernel_sanity_checks();

void recv(size_t len)
{
  received += len;
}

uint64_t ts{0};

void start_measure() {
  received    = 0;
  packets_rx  = Statman::get().get_by_name("eth0.packets_rx").get_uint64();
  packets_tx  = Statman::get().get_by_name("eth0.packets_tx").get_uint64();
  printf("<Settings> DACK: %lld ms WSIZE: %u WS: %u CALC_WIN: %u TS: %s\n",
    dack.count(), winsize, wscale, winsize << wscale, timestamps ? "ON" : "OFF");

  ts          = OS::micros_since_boot();
  printf("<START> %ld\n", ts);
}

void stop_measure() {
  auto diff   = OS::micros_since_boot() - ts;

  kernel_sanity_checks();
  packets_rx  = Statman::get().get_by_name("eth0.packets_rx").get_uint64() - packets_rx;
  packets_tx  = Statman::get().get_by_name("eth0.packets_tx").get_uint64() - packets_tx;
  printf("<STOP> %ld\n", OS::micros_since_boot());
  printf("Packets RX [%lu] TX [%lu]\n", packets_rx, packets_tx);
  double durs   = ((double)diff) / 1000 / 1000;
  double mbits  = (received/(1024*1024)*8) / durs;
  printf("Duration: %.2f s - Payload: %ld/%u MB - %.2f MBit/s\n",
          durs, received/(1024*1024), SIZE/(1024*1024), mbits);

  StackSampler::print(30);
}

void Service::start() {
  kernel_sanity_checks();
}

void Service::ready()
{
  kernel_sanity_checks();
  StackSampler::begin();

  auto& inet = Inet4::ifconfig();
  inet.network_config(
      {  10, 0,  0, 42 },  // IP
      { 255,255,255, 0 },  // Netmask
      {  10, 0,  0,  1 },  // Gateway
      {  10, 0,  0,  1 }); // DNS

  blob = std::make_unique<Chunk>(SIZE);

  static auto& tcp = inet.tcp();
  tcp.set_DACK(dack); // default
  tcp.set_MSL(std::chrono::seconds(3));

  tcp.set_window_size(winsize, wscale);
  tcp.set_timestamps(timestamps);

  tcp.listen(1337).on_connect([](Connection_ptr conn)
  {
    using namespace std::chrono;
    auto id = Timers::periodic(100ms, 500ms,
    [conn](uint32_t)
    {
      //printf("State: %s, Writeq: %u bytes\n",
      //  conn->state().to_string().c_str(), conn->sendq_remaining());
    });

    printf("%s connected. Sending file %u MB\n", conn->remote().to_string().c_str(), SIZE/(1024*1024));
    kernel_sanity_checks();
    start_measure();

    conn->on_close([id]
    {
      Timers::stop(id);

    });
    conn->on_disconnect([] (Connection_ptr self, Connection::Disconnect)
    {
      if(!self->is_closing())
        self->close();
      stop_measure();
    });
    conn->on_write([](size_t n)
    {
      recv(n);
    });
    conn->write(*blob);
    conn->close();
  });

  tcp.listen(1338).on_connect([](Connection_ptr conn)
  {
    using namespace std::chrono;

    printf("%s connected. Receiving file %u MB\n", conn->remote().to_string().c_str(), SIZE/(1024*1024));

    start_measure();

    conn->on_close([]
    {

    });
    conn->on_disconnect([] (Connection_ptr self, Connection::Disconnect)
    {
      if(!self->is_closing())
        self->close();

      stop_measure();
    });
    conn->on_read(16384, [] (buffer_t, size_t n)
    {
      recv(n);
    });
  });
}
