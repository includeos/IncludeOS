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
#include <net/inet4>
#include <statman>
#include <profile>
#include <cstdio>
#include <timers>

#define CLIENT_PORT 1337
#define SERVER_PORT 1338
#define NCAT_RECEIVE_PORT 9000

#define SEND_BUF_LEN 1300
#define SERVER_TEST_LEN 10
#define PACKETS_PER_INTERVAL 50000

using namespace net::ip4;

uint64_t  packets_rx{0};
uint64_t  packets_tx{0};
uint64_t  initial_packets_rx{0};
uint64_t  initial_packets_tx{0};
uint64_t  prev_packets_rx{0};
uint64_t  prev_packets_tx{0};
uint64_t  total_packets_rx{0};
uint64_t  total_packets_tx{0};
uint64_t  data_len{0};
bool      timestamps{true};
bool      first_time{true};
bool      data_received{false};
std::chrono::milliseconds dack{40};
uint64_t  first_ts = 0;
uint64_t  sample_ts = 0;
uint64_t  last_ts = 0;

struct activity {
  void reset() {
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

void init_sample_stats()
{
    data_len = 0;
    initial_packets_rx = Statman::get().get_by_name("eth0.ethernet.packets_rx").get_uint64();
    initial_packets_tx = Statman::get().get_by_name("eth0.ethernet.packets_tx").get_uint64();
    prev_packets_rx  = initial_packets_rx;
    prev_packets_tx = initial_packets_tx;
    first_ts = OS::nanos_since_boot();
    sample_ts = last_ts = first_ts;
    activity_before.reset();
}

void measure_sample_stats()
{
    static uint64_t diff;
    auto prev_diff = diff;

    diff = last_ts - first_ts;
    activity_after.reset();

    activity_after.print(activity_before);

    uint64_t now_rx = Statman::get().get_by_name("eth0.ethernet.packets_rx").get_uint64();
    uint64_t now_tx = Statman::get().get_by_name("eth0.ethernet.packets_tx").get_uint64();

    packets_rx  = now_rx - prev_packets_rx;
    packets_tx  = now_tx - prev_packets_tx;
    prev_packets_rx = now_rx;
    prev_packets_tx = now_tx;
    total_packets_rx = now_rx - initial_packets_rx;
    total_packets_tx = now_tx - initial_packets_tx;

    printf("-------------------Start-----------------\n");
    printf("Packets RX [%llu] TX [%llu]\n", packets_rx, packets_tx);
    printf("Total Packets RX [%llu] TX [%llu]\n", total_packets_rx, total_packets_tx);

    double prev_durs = ((double) (diff - prev_diff) / 1000000000L);
    double total_durs   = ((double)diff) / 1000000000L;
    double mbits  = (data_len/(1024*1024)*8) / total_durs;
    printf("Duration: %.2fs, Total Duration: %.2fs, "
          " Payload: %lld MB %.2f MBit/s\n\n",
          prev_durs, total_durs, data_len /(1024*1024), mbits);
    printf("-------------------End-------------------\n");
}

void send_cb() {
    data_len += SEND_BUF_LEN;
}

void send_data(net::UDPSocket& client, net::Inet<net::IP4>& inet) {
    for (size_t i = 0; i < PACKETS_PER_INTERVAL; i++) {
        const char c = 'A' + (i % 26);
        std::string buff(SEND_BUF_LEN, c);
        client.sendto(inet.gateway(), NCAT_RECEIVE_PORT, buff.data(), buff.size(), send_cb);
    }
    sample_ts = last_ts;
    last_ts = OS::nanos_since_boot();
    printf("Done sending data\n");
}
void Service::start(const std::string& input) {
#ifdef USERSPACE_LINUX
    extern void create_network_device(int N, const char* route, const char* ip);
    create_network_device(0, "10.0.0.0/24", "10.0.0.1");

    // Get the first IP stack configured from config.json
    auto& inet = net::Super_stack::get<net::IP4>(0);
    inet.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});
#else
    auto& inet = net::Super_stack::get<net::IP4>(0);
#endif
    auto& udp = inet.udp();

    if (input.find("client") != std::string::npos) {
        auto& client = udp.bind(CLIENT_PORT);
        printf("Running as Client\n");

        init_sample_stats();
        printf("Sending to %s!\n", inet.gateway().str().c_str());
        send_data(client, inet);
        Timers::periodic(1s, 1s,
          [&inet, &client] (uint32_t timer) {
            static int secs = 0;
            measure_sample_stats();
            send_data(client, inet);
              /* Run the test for 10 seconds */
              if (secs++ == 10) {
                  Timers::stop(timer);
                  printf("Stopping test\n");
              }
          });
    } else {
        auto& server = udp.bind(SERVER_PORT);
        printf("Running as Server. Waiting for data...\n");
        server.on_read(
            []([[maybe_unused]]auto addr,
                      [[maybe_unused]]auto port,
                      const char* buf, int len)
        {
            auto data = std::string(buf, len);
            using namespace std::chrono;
            if (first_time) {
                printf("Received data..\n");
                init_sample_stats();
                first_time = false;
            }
            //CHECK(1, "Getting UDP data from %s:  %d -> %s",
            //    addr.str().c_str(), port, data.c_str());
            data_len += data.size();
            data_received = true;
            sample_ts = last_ts;
            last_ts = OS::nanos_since_boot();
      });

      Timers::periodic(5s, 5s,
      [] (uint32_t) {
        if (data_received) {
            measure_sample_stats();
            data_received = false;
        }
      });
    }
}
