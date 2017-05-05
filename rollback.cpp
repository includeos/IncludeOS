/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 *
**/
#include "liveupdate.hpp"
#include <os>

extern uintptr_t heap_end;

namespace liu
{
  static const char* rollback_data;
  static size_t      rollback_len;

void LiveUpdate::rollback_now()
{
  if (LiveUpdate::has_rollback_blob())
  {
    //printf("\nPerforming rollback from %p:%u...\n",
    //      rollback_data, (uint32_t) rollback_len);
    try
    {
      buffer_t vec(rollback_data, rollback_data + rollback_len);
      // run live update process
      void* ROLLBACK_LOCATION = (void*) (heap_end + 0x4000);
      LiveUpdate::begin(ROLLBACK_LOCATION, std::move(vec));
    }
    catch (std::exception& err)
    {
      fprintf(stderr, "Rollback failed:\n%s\n", err.what());
      OS::reboot();
    }
  }
  else {
    fprintf(stderr, "\nMissing rollback data, rebooting...\n");
    OS::reboot();
  }
}

const std::pair<const char*, size_t> get_rollback_location()
{
  return {rollback_data, rollback_len};
}

void LiveUpdate::set_rollback_blob(const void* buffer, size_t len) noexcept
{
  rollback_data = (const char*) buffer;
  rollback_len  = len;
  OS::on_panic(LiveUpdate::rollback_now);
}
bool LiveUpdate::has_rollback_blob() noexcept
{
                                // minimum legit ELF size
  return rollback_data != nullptr && rollback_len > 164;
}
}

void softreset_service_handler(const void* opaque, size_t length)
{
  // make deep copy?
  auto* data = new char[length];
  memcpy(data, opaque, length);
  liu::rollback_data = data;
  liu::rollback_len  = length;
  OS::on_panic(liu::LiveUpdate::rollback_now);
}
