#include <system_log>
#include <kernel/os.hpp>
#include <ringbuffer>
#include <liveupdate.hpp>
#include <kprint>

static FixedRingBuffer<32768> temp_mrb;
#define MRB_LOG_SIZE (1 << 17)
static MemoryRingBuffer* mrb = nullptr;
static inline RingBuffer* get_mrb()
{
  if (mrb != nullptr) return mrb;
  return &temp_mrb;
}

void SystemLog::write(const char* buffer, size_t length)
{
  size_t free = get_mrb()->free_space();
  if (free < length) {
    get_mrb()->discard(length - free);
  }
  get_mrb()->write(buffer, length);
  // we will want to display the panic, always
  if (OS::is_panicking()) {
    OS::print(buffer, length);
  }
}
std::vector<char> SystemLog::copy()
{
  const auto* buffer = get_mrb()->sequentialize();
  return {buffer, buffer + get_mrb()->size()};
}
void SystemLog::print_all()
{
  const auto* buffer = get_mrb()->sequentialize();
  OS::print(buffer, get_mrb()->size());
}

static void resume_system_log(liu::Restore& store)
{
  mrb = store.as_type<MemoryRingBuffer*> ();
  store.go_next();
}
static void store_system_log(liu::Storage& store, const liu::buffer_t*)
{
  store.add<RingBuffer*> (0, get_mrb());
}

// start dumping the log
extern "C"
void panic_perform_inspection_procedure()
{
  static bool procedure_entered = false;
  if (procedure_entered) return;
  procedure_entered = true;

  SystemLog::print_all();
}

static void register_system_log()
{
  if (liu::LiveUpdate::partition_exists("system_log"))
  {
    liu::LiveUpdate::resume("system_log", resume_system_log);
  }
  else
  {
    // create new MRB
    char* loc = (char*) OS::liveupdate_storage_area() - MRB_LOG_SIZE;
    mrb = (MemoryRingBuffer*) (loc - sizeof(MemoryRingBuffer));
    new (mrb) MemoryRingBuffer(loc, MRB_LOG_SIZE);
    // copy from temp RB
    mrb->write(temp_mrb.sequentialize(), temp_mrb.size());
  }
  liu::LiveUpdate::register_partition("system_log", store_system_log);
}

__attribute__((constructor))
static void system_log_gconstr()
{
  OS::register_plugin(register_system_log, "System log plugin");
}
