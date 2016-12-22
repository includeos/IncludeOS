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
#include <net/inet4>
#include <profile>
#include <util/crc32.hpp>
#include <cstdio>
#include "update.hpp"
#include "hw_timer.hpp"

static void* LIVEUPD_LOCATION   = (void*) 0xC00000; // at 12mb
static const uint16_t TERM_PORT = 6667;

// prevent default serial out
void default_stdout_handlers() {}
#include <hw/serial.hpp>

typedef net::tcp::Connection_ptr Connection_ptr;
static std::vector<Connection_ptr> saveme;
static std::vector<std::string>    savemsg;

void setup_terminal_connection(Connection_ptr conn)
{
  saveme.push_back(conn);
  // retrieve binary
  conn->on_read(512,
  [conn] (net::tcp::buffer_t buf, size_t n)
  {
    std::string str((const char*) buf.get(), n);
    printf("Received message %u: %s", savemsg.size()+1, str.c_str());
    // save for later as strings
    savemsg.push_back(str);
  });
  conn->on_close(
  [conn] {
    printf("Terminal %s closed\n", conn->to_string().c_str());
  });
}

template <typename T>
void setup_terminal(T& inet)
{
  // mini terminal
  printf("Setting up terminal on port %u\n", TERM_PORT);
  
  auto& term = inet.tcp().bind(TERM_PORT);
  term.on_connect(
  [] (auto conn) {
    setup_terminal_connection(conn);
    // write a string to change the state
    char BUFFER_CHAR = 'A';
    static CRC32_BEGIN(crc);
    const int LEN = 4096;
    auto buf = net::tcp::buffer_t(new uint8_t[LEN], std::default_delete<uint8_t[]>());
    
    for (int i = 0; i < 1000; i++) {
      memset(buf.get(), BUFFER_CHAR, LEN);
      conn->write(buf, LEN,
      [conn, buf, LEN] (int) {
        
        crc = crc32(crc, (char*) buf.get(), LEN);
        printf("[%p] CRC32: %08x   %s\n", buf.get(), CRC32_VALUE(crc), conn->to_string().c_str());
      });
      
      //BUFFER_CHAR++;
      if (BUFFER_CHAR > 'Z') BUFFER_CHAR = 'A';
    }
  });
}

template <typename T>
void setup_liveupdate_server(T& inet);

void Service::start(const std::string&)
{
  volatile HW_timer timer("Service::start()");
  // add own serial out after service start
  auto& com1 = hw::Serial::port<1>();
  OS::add_stdout(com1.get_print_handler());

  printf("\n");
  printf("-= Starting LiveUpdate test service =-\n");

  auto& inet = net::Inet4::ifconfig<0>(
        { 10,0,0,42 },     // IP
        { 255,255,255,0 }, // Netmask
        { 10,0,0,1 },      // Gateway
        { 10,0,0,1 });     // DNS


  /// attempt to resume (if there is anything to resume)
  void the_string(Restore);
  void the_buffer(Restore);
  void the_timing(Restore);
  void restore_term(Restore);
  void saved_message(Restore);
  void on_update_area(Restore);
  void on_missing(Restore);

  LiveUpdate::on_resume(1,   the_string);
  LiveUpdate::on_resume(2,   the_buffer);
  LiveUpdate::on_resume(100, the_timing);
  LiveUpdate::on_resume(665, saved_message);
  LiveUpdate::on_resume(666, restore_term);
  LiveUpdate::on_resume(999, on_update_area);
  // begin restoring saved data
  if (LiveUpdate::resume(LIVEUPD_LOCATION, on_missing) == false) {
    printf("* Not restoring data, because no update has happened\n");
    // .. logic for when there is nothing to resume yet
    setup_liveupdate_server(inet);
  }
  
  // listen for telnet clients
  setup_terminal(inet);
}
void Service::ready()
{
  // show profile stats for boot
  printf("%s\n", ScopedProfiler::get_statistics().c_str());
}

#include <hw/cpu.hpp>
void save_stuff(Storage storage, buffer_len final_blob)
{
  storage.add_string(1, "Some string :(");
  storage.add_string(1, "Some other string :(");

  char buffer[] = "Just some random buffer";
  storage.add_buffer(2, {buffer, sizeof(buffer)});

  int64_t ts = hw::CPU::rdtsc();
  storage.add_buffer(100, &ts, sizeof(ts));
  printf("! CPU ticks before: %lld\n", ts);
  
  // where the update was stored last
  printf("Storing location %p:%d\n", final_blob.buffer, final_blob.length);
  storage.add_buffer(999, final_blob.buffer, final_blob.length);
  
  // messages received from terminals
  for (auto& msg : savemsg)
      storage.add_string(665, msg);
  // open terminals
  for (auto conn : saveme)
    if (conn->is_connected())
      storage.add_connection(666, conn);
}

#include <hertz>
#include <chrono>
void the_string(Restore thing)
{
  auto str = thing.as_string();
  printf("The string [some_string] has value [%s]\n", str.c_str());
  assert(str == "Some string :(" || str == "Some other string :(");
}
void the_buffer(Restore thing)
{
  printf("The buffer is %d bytes long\n", thing.length());
  printf("As text: %.*s\n", thing.length(), thing.as_buffer().buffer);
  // there is an extra zero at the end of the buffer
  auto str = std::string(thing.as_buffer().buffer, thing.as_buffer().length-1);
  assert(str == "Just some random buffer");
}
void saved_message(Restore thing)
{
  static int n = 0;
  auto str = thing.as_string();
  
  printf("[%d] %s", ++n, str.c_str());
  // re-save it
  //savemsg.push_back(str);
}
void on_missing(Restore thing)
{
  printf("Missing resume function for %u\n", thing.get_id());
}

void the_timing(Restore thing)
{
  auto t1   = thing.as_type<int64_t>();
  auto t2 = hw::CPU::rdtsc();
  printf("! CPU ticks after: %lld  (CPU freq: %f)\n", t2, OS::cpu_freq().count());

  using namespace std::chrono;
  double  div  = OS::cpu_freq().count() * 1000.0;
  double  time = (t2-t1) / div;

  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
             "! Boot time in ticks: %lld (%.2f ms)\n", t2-t1, time);

  savemsg.emplace_back(buffer, len);
}
void restore_term(Restore thing)
{
  auto& stack = net::Inet4::stack<0> ();
  // restore connection to terminal
  auto conn = thing.as_tcp_connection(stack.tcp());
  setup_terminal_connection(conn);
  printf("Restored terminal connection to %s\n", conn->remote().to_string().c_str());
  
  // send all the messages so far
  //for (auto msg : savemsg)
  //  conn->write(msg);
}

#include <timers>
void on_update_area(Restore thing)
{
  buffer_len updloc = thing.as_buffer().deep_copy();
  printf("Reloading from %p:%d\n", updloc.buffer, updloc.length);
  
  // we are perpetually updating ourselves
  using namespace std::chrono;
  Timers::oneshot(milliseconds(50),
  [updloc] (auto) {
    extern uintptr_t heap_end;
    printf("* Re-running previous update at %p vs heap %#x\n", updloc.buffer, heap_end);
    LiveUpdate::begin(LIVEUPD_LOCATION, updloc, save_stuff);
  });
}


template <typename T>
void setup_liveupdate_server(T& inet)
{
  // listen for live updates
  auto& server = inet.tcp().bind(666);
  server.on_connect(
  [] (auto conn)
  {
    static const int UPDATE_MAX = 1024*1024 * 3; // 3mb files supported
    char* update_blob = new char[UPDATE_MAX];
    int*  update_size = new int(0);

    // retrieve binary
    conn->on_read(9000,
    [conn, update_blob, update_size] (net::tcp::buffer_t buf, size_t n)
    {
      memcpy(update_blob + *update_size, buf.get(), n);
      assert(*update_size + n <= UPDATE_MAX);
      *update_size += (int) n;

    }).on_close(
    [update_blob, update_size] {
      // we received a binary:
      float frac = *update_size / (float) UPDATE_MAX * 100.f;
      printf("* New update size: %u b  (%.2f%%) stored at %p\n", *update_size, frac, update_blob);
      // run live update process
      LiveUpdate::begin(LIVEUPD_LOCATION, {update_blob, *update_size}, save_stuff);
      /// We should never return :-) ///
      assert(0 && "!! Update failed !!");
    });
  });
}
