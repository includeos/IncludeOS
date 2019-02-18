#include <system_log>
#include <kernel.hpp>
#include <os.hpp>
#include <kernel/memory.hpp>
#include <ringbuffer>
#include <kprint>

struct Log_buffer {
  uint64_t magic;
  int32_t  capacity;
  uint32_t flags;
  char     vla[0];

  static const uint64_t MAGIC = 0xDEADC0DEDEADC0DE;

  MemoryRingBuffer* get_mrb()
  { return reinterpret_cast<MemoryRingBuffer*>(&vla[0]); }
};

static FixedRingBuffer<16384> temp_mrb;
#define MRB_AREA_SIZE (65536) // 64kb
#define MRB_LOG_SIZE  (MRB_AREA_SIZE - sizeof(MemoryRingBuffer) - sizeof(Log_buffer))
#define VIRTUAL_MOVE
static MemoryRingBuffer* mrb = nullptr;
static inline RingBuffer* get_mrb()
{
  if (mrb != nullptr) return mrb;
  return &temp_mrb;
}

inline static char* get_system_log_loc()
{
#if defined(ARCH_x86_64) && defined(VIRTUAL_MOVE)
  return (char*) ((1ull << 45) - MRB_AREA_SIZE);
#else
  return (char*) kernel::liveupdate_storage_area() - MRB_AREA_SIZE;
#endif
}
inline static auto* get_ringbuffer_data()
{
  return get_system_log_loc() + sizeof(Log_buffer) + sizeof(MemoryRingBuffer);
}
inline static auto& get_log_buffer()
{
  return *(Log_buffer*) get_system_log_loc();
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

#if defined(ARCH_x86_64) && defined(VIRTUAL_MOVE)
  using namespace util::bitops;
  const uintptr_t syslog_area = (uintptr_t) get_system_log_loc();
  const uintptr_t lu_phys = os::mem::virt_to_phys((uintptr_t) kernel::liveupdate_storage_area());
  // move systemlog to high memory and unpresent physical
  os::mem::virtual_move(lu_phys - MRB_AREA_SIZE, MRB_AREA_SIZE,
                        syslog_area, "SystemLog");
#endif

  auto& buffer = get_log_buffer();
  mrb = buffer.get_mrb();

  // There isn't one, so we have to create
  if(buffer.magic != Log_buffer::MAGIC)
  {
    new (mrb) MemoryRingBuffer(get_ringbuffer_data(), MRB_LOG_SIZE);
    buffer.magic = Log_buffer::MAGIC;
    buffer.capacity = mrb->capacity();
    buffer.flags    = 0;

    INFO2("Created @ %p (%i kB)", get_system_log_loc(), mrb->capacity() / 1024);
    INFO2("Data @ %p (%i bytes)", mrb->data(), mrb->capacity());
  }
  // Correct magic means (hopefully) existing system log
  else
  {
    auto* state = (int*)(&buffer.vla);
    assert(state[0] >= 16);

    new (mrb) MemoryRingBuffer(get_ringbuffer_data(),
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
  os::add_stdout(SystemLog::write);
}
