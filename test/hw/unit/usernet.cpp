
#include <common.cxx>
#include <hw/usernet.hpp>

CASE("UserNet interface")
{
  auto& nic = UserNet::create(64000);
  EXPECT(nic.device_type() == hw::Device::Type::Nic);
  EXPECT(nic.driver_name() == std::string("UserNet"));
  nic.poll();
  nic.flush();
  nic.move_to_this_cpu();
  nic.deactivate();

  EXPECT(nic.create_physical_downstream() != nullptr);
}
