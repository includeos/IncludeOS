#include <system_log>
#include <kernel/os.hpp>
#include <kernel/memory.hpp>
#include <ringbuffer>
#include <kprint>

struct Log_buffer {
  uint64_t magic;
  int32_t  capacity;
  uint32_t flags;
  char     vla[0];

  static const int PADDING = 32;
  static const uint64_t MAGIC = 0xDEADC0DEDEADC0DE;

  MemoryRingBuffer* get_mrb()
  { return reinterpret_cast<MemoryRingBuffer*>(&vla[0]); }
};

static FixedRingBuffer<32768> temp_mrb;
#define MRB_LOG_SIZE (1 << 20)
static MemoryRingBuffer* mrb = nullptr;
static inline RingBuffer* get_mrb()
{
  if (mrb != nullptr) return mrb;
  return &temp_mrb;
}

inline char* get_ringbuffer_loc()
{
#ifdef ARCH_x86_64
  return (char*) ((1ull << 46) - MRB_LOG_SIZE);
#else
  return (char*) OS::liveupdate_storage_area() - MRB_LOG_SIZE;
#endif
}

inline char* get_system_log_loc()
{
  return get_ringbuffer_loc() - sizeof(Log_buffer) - Log_buffer::PADDING;
}

inline Log_buffer& get_log_buffer()
{
  return *((Log_buffer*) get_system_log_loc());
}

uint32_t SystemLog::get_flags()
{
  return get_log_buffer().flags;
}

void SystemLog::set_flags(uint32_t new_flags)
{
  get_log_buffer().flags |= new_flags;
}

void SystemLog::clear_flags()
{
  get_log_buffer().flags = 0;
}

void SystemLog::write(const char* buffer, size_t length)
{
  size_t free = get_mrb()->free_space();
  if (free < length) {
    get_mrb()->discard(length - free);
  }
  get_mrb()->write(buffer, length);
}

std::vector<char> SystemLog::copy()
{
  const auto* buffer = get_mrb()->sequentialize();
  return {buffer, buffer + get_mrb()->size()};
}

void SystemLog::initialize()
{
  INFO("SystemLog", "Initializing System Log");

#ifdef ARCH_x86_64
  using namespace util::bitops;
  const size_t size = MRB_LOG_SIZE - 4096;
  const uintptr_t syslog_area = (uintptr_t) get_ringbuffer_loc() - 4096;
  const uintptr_t lu_phys = os::mem::virt_to_phys((uintptr_t) OS::liveupdate_storage_area());
  // move systemlog to high memory and unpresent physical
  os::mem::virtual_move(lu_phys - size, size, syslog_area, "SystemLog");
#endif

  auto& buffer = get_log_buffer();
  mrb = buffer.get_mrb();

  // There isn't one, so we have to create
  if(buffer.magic != Log_buffer::MAGIC)
  {
    new (mrb) MemoryRingBuffer(get_ringbuffer_loc(), MRB_LOG_SIZE);
    buffer.magic = Log_buffer::MAGIC;
    buffer.capacity = mrb->capacity();
    buffer.flags    = 0;

    INFO2("Created @ %p (%i kB)", mrb->data(), mrb->capacity() / 1024);
  }
  // Correct magic means (hopefully) existing system log
  else
  {
    auto* state = (int*)(&buffer.vla);

    new (mrb) MemoryRingBuffer(get_ringbuffer_loc(),
                    state[0], state[1], state[2], state[3]);

    INFO2("Restored @ %p (%i kB) Flags: 0x%x",
      mrb->data(), mrb->capacity() / 1024, buffer.flags);
  }
  Ensures(mrb != nullptr);
  Expects(buffer.capacity == mrb->capacity());
  Expects(mrb->is_valid());

  // copy from temp RB
  SystemLog::write(temp_mrb.sequentialize(), temp_mrb.size());
}

__attribute__((constructor))
static void system_log_gconstr()
{
  OS::add_stdout(SystemLog::write);
}
