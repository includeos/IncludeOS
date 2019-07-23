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
  static const uint8_t* rollback_data;
  static size_t         rollback_len;

void LiveUpdate::rollback_now(const char* reason)
{
  if (LiveUpdate::has_rollback_blob())
  {
    printf("\nPerforming rollback from %p:%u... Reason: %s\n",
	   rollback_data, (uint32_t) rollback_len, reason);
    try
    {
      // run live update process
      LiveUpdate::exec(rollback_data, rollback_len);
    }
    catch (std::exception& err)
    {
      fprintf(stderr, "Rollback failed:\n%s\n", err.what());
    }
  }
  else {
    fprintf(stderr, "\nMissing rollback data, rebooting...\n");
  }
  fflush(stderr);
  // no matter what, reboot
  os::reboot();
  __builtin_unreachable();
}

const std::pair<const uint8_t*, size_t> get_rollback_location()
{
  return {rollback_data, rollback_len};
}

void LiveUpdate::set_rollback_blob(const void* buffer, size_t len) noexcept
{
  rollback_data = (const uint8_t*) buffer;
  rollback_len  = len;
  os::on_panic(LiveUpdate::rollback_now);
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
  auto* data = new uint8_t[length];
  memcpy(data, opaque, length);
  liu::rollback_data = data;
  liu::rollback_len  = length;
  os::on_panic(liu::LiveUpdate::rollback_now);
}
