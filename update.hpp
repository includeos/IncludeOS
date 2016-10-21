#pragma once
#include <net/inet4>

extern void liveupdate_begin(net::tcp::Connection_ptr);
extern void liveupdate_resume();
