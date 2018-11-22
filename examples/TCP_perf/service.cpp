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
#include <rtc>
#include <net/inet>
#include <statman>
#include <profile>
#include <cstdio>
#include <util/units.hpp>

using namespace net::tcp;

size_t    bufsize = 128*1024;
#ifdef PLATFORM_x86_solo5
static const uint32_t  SIZE = 1024*1024*50;
#else
static const uint32_t  SIZE = 1024*1024*512;
#define ENABLE_JUMBO_FRAMES
#endif
uint64_t  packets_rx{0};
uint64_t  packets_tx{0};
uint64_t  received{0};
uint32_t  winsize{8192};
uint8_t   wscale{5};
bool      timestamps{true};
std::chrono::milliseconds dack{40};
uint64_t  ts = 0;
bool      SACK{true};
bool      keep_last = false;

uint16_t port_send {1337};
uint16_t port_recv {1338};

struct activity {
  void reset() {
#ifdef PLATFORM_x86_solo5
    total  = 0;
    asleep = 0;
#else
    total  = StackSampler::samples_total();
    asleep = StackSampler::samples_asleep();
#endif
  }
  void print(activity& other) {
    auto tdiff = total - other.total;
    auto sdiff = asleep - other.asleep;
    if (tdiff > 0) {
      double idle = sdiff / (float) tdiff;
      printf("* CPU was %.2f%% idle\n", idle * 100.0);
    }
  }
  uint64_t total;
  uint64_t asleep;
};
activity activity_before;
activity activity_after;

void recv(size_t len)
{
  received += len;
}

void start_measure()
{
  received    = 0;
  packets_rx  = Statman::get().get_by_name("eth0.ethernet.packets_rx").get_uint64();
  packets_tx  = Statman::get().get_by_name("eth0.ethernet.packets_tx").get_uint64();
  printf("<Settings> BUFSZ=%zukB DACK=%llims WSIZE=%u WS=%u CALC_WIN=%ukB TS=%s SACK=%s\n",
    bufsize/1024,
    dack.count(), winsize, wscale, (winsize << wscale)/1024,
    timestamps ? "ON" : "OFF",
    SACK ? "ON" : "OFF");
  ts          = RTC::nanos_now();
  activity_before.reset();
}

void stop_measure()
{
  auto diff   = RTC::nanos_now() - ts;
  activity_after.reset();

#ifndef PLATFORM_x86_solo5
  StackSampler::print(15);
#endif
  activity_after.print(activity_before);

  packets_rx  = Statman::get().get_by_name("eth0.ethernet.packets_rx").get_uint64() - packets_rx;
  packets_tx  = Statman::get().get_by_name("eth0.ethernet.packets_tx").get_uint64() - packets_tx;
  printf("Packets RX [%lu] TX [%lu]\n", packets_rx, packets_tx);
  double durs   = (double) diff / 1000000000ULL;
  double mbits  = (double(received)/(1024*1024)*8) / durs;

  printf("Duration: %.3fs - Payload: %s (Generated size %s) - %.2f MBit/s\n",
         durs,
         util::Byte_r(received).to_string().c_str(),
         util::Byte_r(SIZE).to_string().c_str(), mbits);

}

void Service::start(const std::string& args) {

  if (args.find("keep") != args.npos) {
    printf(">>> Keeping last received file \n");
    keep_last = true;
  }
}

net::tcp::buffer_t blob = nullptr;

struct file {
  using Buf = net::tcp::buffer_t;
  using Vec = std::vector<Buf>;

  auto begin() { return chunks.begin(); }
  auto end()   { return chunks.end();   }

  size_t size(){ return sz; }
  size_t blkcount() { return chunks.size(); }

  void append(Buf& b) {
    chunks.push_back(b);
    sz += b->size();
  }

  void reset() {
    chunks.clear();
    sz = 0;
  }

  Vec chunks{};
  size_t sz{};
};

file filerino;

void Service::ready()
{
#ifndef PLATFORM_x86_solo5
  StackSampler::begin();
  StackSampler::set_mode(StackSampler::MODE_DUMMY);
#endif

  blob = net::tcp::construct_buffer(SIZE, '!');

#ifdef USERSPACE_LINUX
  extern void create_network_device(int N, const char* route, const char* ip);
  create_network_device(0, "10.0.0.0/24", "10.0.0.1");
#endif

  // Get the first IP stack configured from config.json
  auto& inet = net::Super_stack::get(0);
  auto& tcp = inet.tcp();
  tcp.set_DACK(dack); // default
  tcp.set_MSL(std::chrono::seconds(3));

  tcp.set_window_size(winsize, wscale);
  tcp.set_timestamps(timestamps);
  tcp.set_SACK(SACK);

  tcp.listen(port_send).on_connect([](Connection_ptr conn)
  {
    printf("%s connected. Sending file %u MB\n", conn->remote().to_string().c_str(), SIZE/(1024*1024));
    start_measure();

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

    if (! keep_last) {
      conn->write(blob);
    } else {
      for (auto b : filerino)
        conn->write(b);
    }
    conn->close();
  });

  tcp.listen(port_recv).on_connect([](net::tcp::Connection_ptr conn)
  {
    using namespace std::chrono;
    printf("%s connected. Receiving file %u MB\n", conn->remote().to_string().c_str(), SIZE/(1024*1024));
    filerino.reset();

    start_measure();

    conn->on_close([]
    {

    });
    conn->on_disconnect([] (net::tcp::Connection_ptr self,
                            net::tcp::Connection::Disconnect reason)
    {
      (void) reason;
      if(const auto bytes_sacked = self->bytes_sacked(); bytes_sacked)
        printf("SACK: %zu bytes (%zu kB)\n", bytes_sacked, bytes_sacked/(1024));

      if(!self->is_closing())
        self->close();

      stop_measure();
    });
    conn->on_read(SIZE, [] (buffer_t buf)
    {
      recv(buf->size());
      if (UNLIKELY(keep_last)) {
        filerino.append(buf);
      }
    });
  });
}

#ifdef ENABLE_JUMBO_FRAMES
#include <hw/nic.hpp>
namespace hw {
  uint16_t Nic::MTU_detection_override(int idx, const uint16_t default_MTU)
  {
    if (idx == 0) return 9000;
    return default_MTU;
  }
}
#endif
