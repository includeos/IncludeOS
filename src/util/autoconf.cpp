
#include <util/autoconf.hpp>
#include <util/config.hpp>
#include <net/configure.hpp>
#include <common>

void autoconf::run()
{
  INFO("Autoconf", "Running auto configure");

  const auto& cfg = Config::get();

  if(cfg.empty())
  {
    INFO2("No config found");
    return;
  }

  const auto& doc = Config::doc();
  // Configure network
  if(doc.HasMember("net"))
  {
    net::configure(doc["net"]);
  }

  INFO("Autoconf", "Finished");
}
