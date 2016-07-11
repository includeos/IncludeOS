// License

#include <os> // uptime
#include <net/tcp/rttm.hpp>

using namespace net::tcp;

const RTTM::duration_t RTTM::CLOCK_G;

void RTTM::start() {
  t = OS::uptime();
  active = true;
}

void RTTM::stop(bool first) {
  assert(active);
  active = false;
  // round trip time (RTT)
  auto rtt = OS::uptime() - t;
  debug2("<RTTM::stop> RTT: %ums\n",
    (uint32_t)(rtt * 1000));
  if(!first)
    sub_rtt_measurement(rtt);
  else {
    first_rtt_measurement(rtt);
  }
}
