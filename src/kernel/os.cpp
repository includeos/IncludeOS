
#include <os.hpp>
#include <kernel.hpp>

bool os::is_booted() noexcept {
  return kernel::is_booted();
}
const char* os::arch() noexcept {
  return Arch::name;
}

os::Panic_action os::panic_action() noexcept {
  return kernel::panic_action();
}

void os::set_panic_action(Panic_action action) noexcept {
  kernel::set_panic_action(action);
}

os::Span_mods os::modules()
{
  auto* bootinfo_ = kernel::bootinfo();
  if (bootinfo_ and bootinfo_->flags & MULTIBOOT_INFO_MODS and bootinfo_->mods_count) {

    Expects(bootinfo_->mods_count < std::numeric_limits<int>::max());

    return os::Span_mods {
      reinterpret_cast<os::Module*>(bootinfo_->mods_addr),
        static_cast<int>(bootinfo_->mods_count) };
  }
  return {};
}
