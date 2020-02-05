#include <os.hpp>
#include <kernel.hpp>
#include <kernel/cpuid.hpp>
#include <kernel/rng.hpp>
#include <service>
#include <cstdio>
#include <cinttypes>
#include <util/fixed_vector.hpp>
#include <system_log>
#define MYINFO(X,...) INFO("Kernel", X, ##__VA_ARGS__)
//#define ENABLE_PROFILERS
#include <profile>

using namespace util;

extern char _start;
extern char _end;
extern char _ELF_START_;
extern char _TEXT_START_;
extern char _LOAD_START_;
extern char _ELF_END_;

extern char __for_production_use;
inline static bool is_for_production_use() {
  return &__for_production_use == (char*) 0x2000;
}

kernel::State& kernel::state() noexcept {
    static kernel::State state;
    return state;
}

util::KHz os::cpu_freq() {
  return kernel::cpu_freq();
}

// stdout redirection
static Fixed_vector<os::print_func, 16> os_print_handlers;

// Plugins
struct Plugin_desc {
  Plugin_desc(os::Plugin f, const char* n) : func{f}, name{n} {}

  os::Plugin  func;
  const char* name;
};
static Fixed_vector<Plugin_desc, 64> plugins;

const char* os::cmdline_args() noexcept {
  return kernel::cmdline();
}

extern kernel::ctor_t __plugin_ctors_start;
extern kernel::ctor_t __plugin_ctors_end;
extern kernel::ctor_t __service_ctors_start;
extern kernel::ctor_t __service_ctors_end;

void os::register_plugin(Plugin delg, const char* name){
  MYINFO("Registering plugin %s", name);
  plugins.emplace_back(delg, name);
}

extern void __arch_reboot();
void os::reboot() noexcept
{
  __arch_reboot();
}
void os::shutdown() noexcept
{
  kernel::state().running = false;
}

void kernel::post_start()
{
  // Enable timestamps (if present)
  kernel::state().timestamps_ready = true;

  {
	PROFILE("LiveUpdate and SystemLog");
    // LiveUpdate needs some initialization, although only if present
    kernel::setup_liveupdate();

    // Initialize the system log if plugin is present.
    // Dependent on the liveupdate location being set
    SystemLog::initialize();
  }

  // Seed rand with 32 bits from RNG
  srand(rng_extract_uint32());

  // Custom initialization functions
  MYINFO("Initializing plugins");
  {
	PROFILE("Plugin constructors");
    kernel::run_ctors(&__plugin_ctors_start, &__plugin_ctors_end);

    // Run plugins
    for (auto plugin : plugins) {
      INFO2("* Initializing %s", plugin.name);
      plugin.func();
    }
  }

  MYINFO("Running service constructors");
  FILLINE('-');
  {
	PROFILE("Service constructors");
    // the boot sequence is over when we get to plugins/Service::start
    kernel::state().boot_sequence_passed = true;

    // Run service constructors
    kernel::run_ctors(&__service_ctors_start, &__service_ctors_end);
  }

  // begin service start
  FILLINE('=');
  printf(" IncludeOS %s (%s / %u-bit)\n",
         os::version(), os::arch(),
         static_cast<unsigned>(sizeof(uintptr_t)) * 8);
  printf(" +--> Running [ %s ]\n", Service::name());
  FILLINE('~');

  // if we have disabled important checks, its unsafe for production
#if defined(LIBFUZZER_ENABLED) || defined(ARP_PASSTHROUGH) || defined(DISABLE_INET_CHECKSUMS)
  const bool unsafe = true;
#else
  // if we dont have a good random source, its unsafe for production
  const bool unsafe = !CPUID::has_feature(CPUID::Feature::RDSEED)
                   && !CPUID::has_feature(CPUID::Feature::RDRAND);
#endif
  if (unsafe) {
    printf(" +--> WARNING: No good random source found: RDRAND/RDSEED instructions not available.\n");
    if (is_for_production_use()) {
      printf(" +--> FATAL: Random source check failed. Terminating.\n");
      printf(" +-->        To disable this check, re-run cmake with \"-DFOR_PRODUCTION=OFF\".\n");
      os::shutdown();
      return;
    }
    FILLINE('~');
  }

  // service program start
  {
	PROFILE("Service::start");
    Service::start();
  }
}

void os::add_stdout(os::print_func func)
{
  os_print_handlers.push_back(func);
}

void os::default_stdout(const char* str, size_t len)
{
  kernel::default_stdout(str, len);
}
__attribute__((weak))
bool os_enable_boot_logging = false;
__attribute__((weak))
bool os_default_stdout = false;

#include <isotime>
static inline bool contains(const char* str, size_t len, char c)
{
  for (size_t i = 0; i < len; i++) if (str[i] == c) return true;
  return false;
}

void os::print(const char* str, const size_t len)
{
  if (UNLIKELY(! kernel::libc_initialized())) {
    kernel::default_stdout(str, len);
    return;
  }

  /** TIMESTAMPING **/
  if (kernel::timestamps() && kernel::timestamps_ready() && !kernel::is_panicking())
  {
    static bool apply_ts = true;
    if (apply_ts)
    {
      std::string ts = "[" + isotime::now() + "] ";
      for (const auto& callback : os_print_handlers) {
        callback(ts.c_str(), ts.size());
      }
      apply_ts = false;
    }
    const bool has_newline = contains(str, len, '\n');
    if (has_newline) apply_ts = true;
  }
  /** TIMESTAMPING **/

  if (os_enable_boot_logging || kernel::is_booted() || kernel::is_panicking())
  {
    for (const auto& callback : os_print_handlers) {
      callback(str, len);
    }
  }
}

void os::print_timestamps(const bool enabled)
{
  kernel::state().timestamps = enabled;
}

#include <kernel/mrspinny.hpp>
struct struct_spinny mr_spinny {};
