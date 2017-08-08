/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#include <service>
#include <net/inet4>
#include <cstdio>
#include "liveupdate.hpp"
#include "common.hpp"

using storage_func_t = liu::LiveUpdate::storage_func;
void setup_liveupdate_server(net::Inet<net::IP4>&, storage_func_t);

extern storage_func_t begin_test_all(net::Inet<net::IP4>&);
extern storage_func_t begin_test_boot();
extern storage_func_t begin_test_tcpflow(net::Inet<net::IP4>&);

void Service::start()
{
  printf("\n");
  printf("-= Starting LiveUpdate test service =-\n");
  auto func = begin_test_boot();

  if (liu::LiveUpdate::is_resumable(LIVEUPD_LOCATION) == false)
  {
    auto& inet = net::Inet4::ifconfig<0>(
          { 10,0,0,42 },     // IP
          { 255,255,255,0 }, // Netmask
          { 10,0,0,1 },      // Gateway
          { 10,0,0,1 });     // DNS

    //auto func = begin_test_tcpflow(inet);
    setup_liveupdate_server(inet, func);
  }
}

#include "server.hpp"
void setup_liveupdate_server(net::Inet<net::IP4>& inet, liu::LiveUpdate::storage_func func)
{
  static liu::LiveUpdate::storage_func save_function;
  save_function = func;

  // listen for live updates
  server(inet, 666,
  [] (liu::buffer_t& buffer)
  {
    printf("* Live updating from %p (len=%u)\n",
            buffer.data(), (uint32_t) buffer.size());
    try
    {
      // run live update process
      liu::LiveUpdate::begin(LIVEUPD_LOCATION, buffer, save_function);
    }
    catch (std::exception& err)
    {
      liu::LiveUpdate::restore_environment();
      printf("Live Update location: %p\n", LIVEUPD_LOCATION);
      show_heap_stats();
      printf("Live update failed:\n%s\n", err.what());
    }
  });
  // listen for rollback blobs
  server(inet, 665,
  [] (liu::buffer_t& buffer)
  {
    static liu::buffer_t rb_blob;
    rb_blob = buffer;
    liu::LiveUpdate::set_rollback_blob(rb_blob.data(), rb_blob.size());
    // simulate crash
    liu::LiveUpdate::rollback_now("Network triggered rollback");
  });

  printf("LiveUpdate server listening on port 666\n");
}
