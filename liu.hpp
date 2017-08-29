#pragma once
#include <liveupdate>
#include "../LiveUpdate/server.hpp"

void setup_liveupdate_server(net::Inet<net::IP4>& inet, const uint16_t PORT, liu::LiveUpdate::storage_func func)
{
  static liu::LiveUpdate::storage_func save_function;
  save_function = func;

  // listen for live updates
  server(inet, PORT,
  [] (liu::buffer_t& buffer)
  {
    printf("* Live updating from %p (len=%u)\n",
            buffer.data(), (uint32_t) buffer.size());
    try
    {
      // run live update process
      liu::LiveUpdate::begin(buffer);
    }
    catch (std::exception& err)
    {
      liu::LiveUpdate::restore_environment();
      printf("Live update failed:\n%s\n", err.what());
    }
  });
}
