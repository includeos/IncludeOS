
#include <net/interfaces.hpp>
#include <hal/machine.hpp>

namespace net
{

Inet& Interfaces::create(hw::Nic& nic, int N, int sub)
{
  INFO("Network", "Creating stack for %s on %s (MTU=%u)",
        nic.driver_name(), nic.device_name().c_str(), nic.MTU());

  auto& stacks = instance().stacks_.at(N);

  auto it = stacks.find(sub);
  if(it != stacks.end() and it->second != nullptr) {
    throw Interfaces_err{"Stack already exists ["
      + std::to_string(N) + "," + std::to_string(sub) + "]"};
  }

  auto inet = [&nic]()->auto {
    switch(nic.proto()) {
    case hw::Nic::Proto::ETH:
      return std::make_unique<Inet>(nic);
    default:
      throw Interfaces_err{"Nic not supported"};
    }
  }();

  Ensures(inet != nullptr);

  stacks[sub] = std::move(inet);

  return *stacks[sub];
}

Inet& Interfaces::get(int N)
{
  if (N < 0 || N >= os::machine().count<hw::Nic>())
    throw Stack_not_found{"No IP4 stack found with index: " + std::to_string(N) +
      ". Missing device (NIC) or driver."};

  auto& stacks = instance().stacks_.at(N);

  if(stacks[0] != nullptr)
    return *stacks[0];

  // create network stack
  auto& nic = os::machine().get<hw::Nic>(N);
  return instance().create(nic, N, 0);
}

Inet& Interfaces::get(int N, int sub)
{
  if (N < 0 || N >= os::machine().count<hw::Nic>())
    throw Stack_not_found{"No IP4 stack found with index: " + std::to_string(N) +
      ". Missing device (NIC) or driver."};

  auto& stacks = instance().stacks_.at(N);

  auto it = stacks.find(sub);

  if(it != stacks.end()) {
    Expects(it->second != nullptr && "Creating empty subinterfaces doesn't make sense");
    return *it->second;
  }

  throw Stack_not_found{"IP4 Stack not found ["
      + std::to_string(N) + "," + std::to_string(sub) + "]"};
}

hw::Nic& Interfaces::get_nic(int idx)
{
  try
  {
    return os::machine().get<hw::Nic>(idx);
  }
  catch(...)
  {
    throw Interfaces_err{"No NIC found with index " + std::to_string(idx)};
  }
}

hw::Nic& Interfaces::get_nic(const MAC::Addr& mac)
{
  try
  {
    for(auto& nic : os::machine().get<hw::Nic>())
    {
      if (nic.get().mac() == mac)
        return nic;
    }
  }
  catch(const std::runtime_error& err)
  {
    throw Interfaces_err{"No NICs found: " + std::string{err.what()}};
  }

  throw Interfaces_err{"No NIC found with MAC address " + mac.to_string()};
}

int Interfaces::get_nic_index(const MAC::Addr& mac)
{
  int index = -1;
  auto nics = os::machine().get<hw::Nic>();
  for (size_t i = 0; i < nics.size(); i++) {
    const hw::Nic& nic = nics.at(i);
    if (nic.mac() == mac) {
      index = i;
      break;
    }
  }

  return index;
}

Inet& Interfaces::get(const std::string& mac)
{
  auto index = get_nic_index(mac);
  if(index < 0)
    throw Interfaces_err{"No NIC found with MAC address " + mac};

  auto& stacks = instance().stacks_.at(index);
  auto& stack = stacks[0];
  if(stack != nullptr) {
    Expects(stack->link_addr() == MAC::Addr(mac));
    return *stack;
  }

  // If not found, create
  return instance().create(os::machine().get<hw::Nic>(index), index, 0);
}

// Duplication of code to keep sanity intact
Inet& Interfaces::get(const std::string& mac, int sub)
{
  auto index = get_nic_index(mac);
  return get(index, sub);
}

Interfaces::Interfaces()
{
  if (os::machine().count<hw::Nic>() == 0)
    INFO("Network", "No registered network interfaces found");

  for (size_t i = 0; i < os::machine().get<hw::Nic>().size(); i++) {
    stacks_.emplace_back();
    stacks_.back()[0] = nullptr;
  }
}

}
