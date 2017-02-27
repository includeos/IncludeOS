#include "liveupdate.hpp"
#include <os>

extern uintptr_t heap_end;

namespace liu
{
  static buffer_len rollback_data = {nullptr, 0};

void LiveUpdate::rollback_now()
{
  if (LiveUpdate::has_rollback_blob())
  {
    //printf("\nPerforming rollback from %p...\n", rollback_data.buffer);
    try
    {
      void* ROLLBACK_LOCATION = (void*) (heap_end + 0x4000);
      // run live update process
      LiveUpdate::begin(ROLLBACK_LOCATION, rollback_data);
    }
    catch (std::exception& err)
    {
      printf("Rollback failed:\n%s\n", err.what());
    }
    
  }
  else {
    printf("\nMissing rollback data, rebooting...\n");
    OS::reboot();
  }
}

buffer_len get_rollback_location()
{
  return rollback_data;
}

void LiveUpdate::set_rollback_blob(buffer_len buffer) noexcept
{
  rollback_data = buffer;
  OS::on_panic(LiveUpdate::rollback_now);
}
bool LiveUpdate::has_rollback_blob() noexcept
{
  return rollback_data.buffer != nullptr && rollback_data.length > 164;
}
}

void softreset_service_handler(const void* opaque, size_t length)
{
  liu::rollback_data.buffer = (const char*) opaque;
  liu::rollback_data.length = length;
  OS::on_panic(liu::LiveUpdate::rollback_now);
}
