#include "client.hpp"
#include "ircd.hpp"
#include <rtc>

// ping timeout QUIT message
static const std::string reason(
  "Ping timeout: " + std::to_string(IrcServer::ping_timeout().count()) + " seconds");

void Client::handle_timeout()
{
  assert(this->is_alive());

  if (this->is_warned() == false)
  {
    // create and send ping request
    char pingreq[32];
    int len = snprintf(pingreq, sizeof(pingreq), "PING :%ld\r\n", RTC::now());
    this->send_raw(pingreq, len);
    // set warned
    this->set_warned(true);
    // registered clients get longer timeouts
    const auto d = this->is_reg() ? server.ping_timeout() : server.short_ping_timeout();
    to_timer.restart(d, {this, &Client::handle_timeout});
  }
  else {
    // kick client for being unresponsive
    this->kill(false, reason);
  }
}

void Client::restart_timeout()
{
  if (this->is_warned()) {
    to_timer.restart(server.short_ping_timeout(), {this, &Client::handle_timeout});
  }
  else {
    to_timer.restart(server.ping_timeout(), {this, &Client::handle_timeout});
  }
}
