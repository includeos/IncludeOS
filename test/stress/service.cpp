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
#include <net/interfaces>
#include <net/dhcp/dh4client.hpp>
#include <math.h> // rand()
#include <sstream>
#include <timers>
#include <statman>

using namespace std::chrono;


std::string header(int content_size) {
  std::string head="HTTP/1.1 200 OK \n "                \
    "Date: Mon, 01 Jan 1970 00:00:01 GMT \n"            \
    "Server: IncludeOS prototype 4.0 \n"                \
    "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT \n"     \
    "Content-Type: text/html; charset=UTF-8 \n"            \
    "Content-Length: "+std::to_string(content_size)+"\n"   \
    "Accept-Ranges: bytes\n"                               \
    "Connection: close\n\n";

  return head;
}

std::string html() {
  int color = rand();
  std::stringstream stream;

  /* HTML Fonts */
  std::string ubuntu_medium  = "font-family: \'Ubuntu\', sans-serif; font-weight: 500; ";
  std::string ubuntu_normal  = "font-family: \'Ubuntu\', sans-serif; font-weight: 400; ";
  std::string ubuntu_light  = "font-family: \'Ubuntu\', sans-serif; font-weight: 300; ";

  /* HTML */
  stream << "<html><head>"
         << "<link href='https://fonts.googleapis.com/css?family=Ubuntu:500,300' rel='stylesheet' type='text/css'>"
         << "</head><body>"
         << "<h1 style= \"color: " << "#" << std::hex << (color >> 8) << "\">"
         <<  "<span style=\""+ubuntu_medium+"\">Include</span><span style=\""+ubuntu_light+"\">OS</span> </h1>"
         <<  "<h2>Now speaks TCP!</h2>"
    // .... generate more dynamic content
         << "<p> This is improvised http, but proper suff is in the works. </p>"
         << "<footer><hr /> &copy; 2016, IncludeOS AS @ 60&deg; north </footer>"
         << "</body></html>\n";

  std::string html = stream.str();

  return html;
}

const std::string NOT_FOUND = "HTTP/1.1 404 Not Found \n Connection: close\n\n";

uint64_t TCP_BYTES_RECV = 0;
uint64_t TCP_BYTES_SENT = 0;

void print_memuse(uintptr_t u) {
  auto bytes_used = os::total_memuse();
  auto kb_used = bytes_used / 1024;

  printf("Current memory usage: %s (%zi b)  total memuse: %zu (%zu kb)\n",
         util::Byte_r(u).to_string().c_str(), u, bytes_used, kb_used);
}

void Service::start(const std::string&)
{
  using namespace util::literals;
  // Allocation / free spam to warm up
  auto initial_memuse =  os::total_memuse();
  auto initial_highest_used = initial_memuse;
  print_memuse(initial_memuse);

  std::array<volatile void*, 10> allocs {};
  auto chunksize = 1024 * 1024 * 5;
  printf("Exercising heap: incrementally allocating %zi x %i bytes \n",
         allocs.size(), chunksize);

  for (auto& ptr : allocs) {
    ptr = malloc(chunksize);
    Expects(ptr != nullptr);
    memset((void*)ptr, '!', chunksize);
    for (char* c = (char*)ptr; c < (char*)ptr + chunksize; c++)
      Expects(*c == '!');
    printf("Allocated area: %p\n", (void*)ptr);
    auto memuse = os::total_memuse();

    print_memuse(memuse);
    Expects(memuse > initial_memuse);
  }

  // Verify new used heap area covers recent heap growth
  Expects(os::total_memuse() - initial_highest_used >=
          os::total_memuse() - initial_memuse);

  auto high_memuse = os::total_memuse();
  Expects(high_memuse >= (chunksize * allocs.size()) + initial_memuse);

  auto prev_memuse = high_memuse;

  printf("Deallocating \n");
  for (auto& ptr : allocs) {
    free((void*)ptr);
    auto memuse = os::total_memuse();
    print_memuse(memuse);
    Expects(memuse < high_memuse);
    Expects(memuse < prev_memuse);
    prev_memuse = memuse;
  }

  // Expect the heap to have shrunk. With musl and chunksize of 5Mib,
  // the mallocs will have resulted in calls to mmap, and frees in calls to
  // munmap, so we could expect to be back to exacty where we were, but
  // we're adding some room (somewhat arbitrarily) for malloc to change and
  // not necessarily give back all it allocated.
  Expects(os::total_memuse() <= initial_memuse + 1_MiB);
  printf("Heap functioning as expected\n");

  // Timer spam
  for (int i = 0; i < 1000; i++)
    Timers::oneshot(std::chrono::microseconds(i + 200), [](auto){});

  static auto& inet = net::Interfaces::get(0);

  // Static IP configuration, until we (possibly) get DHCP
  // @note : Mostly to get a robust demo service that it works with and without DHCP
  inet.network_config( { 10,0,0,42 },      // IP
                       { 255,255,255,0 },  // Netmask
                       { 10,0,0,1 },       // Gateway
                       { 8,8,8,8 } );      // DNS

  srand(os::cycles_since_boot());

  // Set up a TCP server
  auto& server = inet.tcp().listen(80);
  inet.tcp().set_MSL(5s);
  auto& server_mem = inet.tcp().listen(4243);

  // Set up a UDP server
  net::UDP::port_t port = 4242;
  auto& conn = inet.udp().bind(port);

  net::UDP::port_t port_mem = 4243;
  auto& conn_mem = inet.udp().bind(port_mem);


  Timers::periodic(1s, 10s,
  [] (Timers::id_t) {
    auto memuse =  os::total_memuse();
    printf("Current memory usage: %i b, (%f MB) \n", memuse, float(memuse)  / 1000000);
    printf("Recv: %llu Sent: %llu\n", TCP_BYTES_RECV, TCP_BYTES_SENT);
    printf("eth0.sendq_max: %zu, eth0.sendq_now: %zu"
           "eth0.stat_rx_total_packets: %zu, eth0.stat_rx_total_packets: %zu, "
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
  });

  server_mem.on_connect([] (net::tcp::Connection_ptr conn) {
      conn->on_read(1024, [conn](net::tcp::buffer_t buf) {
          TCP_BYTES_RECV += buf->size();
          // create string from buffer
          std::string received { (char*) buf->data(), buf->size() };
          auto reply = std::to_string(os::total_memuse())+"\n";
          // Send the first packet, and then wait for ARP
          printf("TCP Mem: Reporting memory size as %s bytes\n", reply.c_str());
          conn->on_write([](size_t n) {
            TCP_BYTES_SENT += n;
          });
          conn->write(reply);

          conn->on_disconnect([](net::tcp::Connection_ptr c, auto){
              c->close();
            });
        });
    });



  // Add a TCP connection handler - here a hardcoded HTTP-service
  server.on_connect([] (net::tcp::Connection_ptr conn) {
        // read async with a buffer size of 1024 bytes
        // define what to do when data is read
        conn->on_read(1024, [conn](net::tcp::buffer_t buf) {
            TCP_BYTES_RECV += buf->size();
            // create string from buffer
            std::string data { (char*) buf->data(), buf->size() };

            if (data.find("GET / ") != std::string::npos) {

              auto htm = html();
              auto hdr = header(htm.size());


              // create response
              conn->write(hdr);
              conn->write(htm);
            }
            else {
              conn->write(NOT_FOUND);
            }
          });
        conn->on_write([](size_t n) {
          TCP_BYTES_SENT += n;
        });

      });

  // UDP connection handler
  conn.on_read([&] (net::UDP::addr_t addr, net::UDP::port_t port, const char* data, int len) {
      std::string received = std::string(data,len);
      std::string reply = received;

      // Send the first packet, and then wait for ARP
      conn.sendto(addr, port, reply.c_str(), reply.size());
    });

  // UDP utility to return memory usage
  conn_mem.on_read([&] (net::UDP::addr_t addr, net::UDP::port_t port, const char* data, int len) {
      std::string received = std::string(data,len);
      Expects(received == "memsize");
      auto reply = std::to_string(os::total_memuse());
      // Send the first packet, and then wait for ARP
      printf("Reporting memory size as %s bytes\n", reply.c_str());
      conn.sendto(addr, port, reply.c_str(), reply.size());
    });



  printf("*** TEST SERVICE STARTED *** \n");
  auto memuse = os::total_memuse();
  printf("Current memory usage: %zi b, (%f MB) \n", memuse, float(memuse)  / 1000000);

  /** These printouts are event-triggers for the vmrunner **/
  printf("Ready to start\n");
  printf("Ready for ARP\n");
  printf("Ready for UDP\n");
  printf("Ready for ICMP\n");
  printf("Ready for TCP\n");
  printf("Ready to end\n");
}
