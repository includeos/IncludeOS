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
#include <smp> // SMP class

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

std::string HTML_RESPONSE() {
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

  std::string header="HTTP/1.1 200 OK \n "        \
    "Date: Mon, 01 Jan 1970 00:00:01 GMT \n"      \
    "Server: IncludeOS prototype 4.0 \n"                \
    "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT \n"   \
    "Content-Type: text/html; charset=UTF-8 \n"     \
    "Content-Length: "+std::to_string(html.size())+"\n"   \
    "Accept-Ranges: bytes\n"          \
    "Connection: close\n\n";
  return header + html;
}

#include <kernel/elf.hpp>
#include <debug_new>
#include <debug_shared>
#include <profile>

void test_debugging()
{
  auto* testint = NEW(int);
  auto* testarr = NEW_ARRAY(int, 24);
  
  debug_new_print_entries();
  
  DELETE(testint);
  DELETE_ARRAY(testarr);
  
  debug_new_print_entries();
  
  auto test = make_debug_shared<int> (4);
  auto crash = debug_ptr<int> (nullptr, [] (void*) { printf("custom deleter\n"); });
  *crash;
}

using namespace std::chrono;

void Service::start()
{
  //void begin_work();
  //begin_work();
  //test_debugging();
  
  // start sampling, gather every X ms
  begin_stack_sampling(1500);
  // print stuff every 5 seconds
  hw::PIT::instance().on_repeated_timeout(5000ms,
  [] { print_stack_sampling(15); });
  
  // boilerplate
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> > (eth0, 1);
  inet->network_config(
    { 10,0,0,42 },      // IP
    { 255,255,255,0 },  // Netmask
    { 10,0,0,1 },       // Gateway
    { 8,8,8,8 } );      // DNS

  // Set up a TCP server on port 80
  auto& server = inet->tcp().bind(80);
  // Add a TCP connection handler - here a hardcoded HTTP-service
  server.onAccept([] (auto conn) -> bool {
      return true; // allow all connections

    })
    .onConnect([] (auto conn) {
      // read async with a buffer size of 1024 bytes
      // define what to do when data is read
      conn->read(1024, [conn](net::TCP::buffer_t buf, size_t n) {
          // create string from buffer
          std::string data { (char*)buf.get(), n };

          if (data.find("GET / ") != std::string::npos) {
            // create response
            std::string response = HTML_RESPONSE();
            // write the data from the string with the strings size
            conn->write(response.data(), response.size(), 
            [] (size_t n)
            {
                (void) n;
            });
          }
        });

    })
    .onDisconnect(
    [] (auto conn, auto) {
        conn->close();
    })
    .onError([](auto, auto err) {
      printf("<Service> @onError - %s\n", err.what());
    });
  
  printf("*** TEST SERVICE STARTED *** \n");
}

void begin_work()
{
  static const int TASKS = 32;
  static uint32_t completed = 0;
  static uint32_t job;
  
  job = 0;
  
  // schedule tasks
  for (int i = 0; i < TASKS; i++)
  SMP::add_task(
  [i] {
    __sync_fetch_and_or(&job, 1 << i);
  }, 
  [i] {
    printf("completed task %d\n", completed);
    completed++;
    
    if (completed % TASKS == 0) {
      printf("All jobs are done now, compl = %d\t", completed);
      printf("bits = %#x\n", job);
      print_backtrace();
    }
  });
  // start working on tasks
  SMP::start();
}
