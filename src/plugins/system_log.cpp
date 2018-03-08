#include <system_log>
#include <kernel/os.hpp>
#include <ringbuffer>
#include <kprint>

struct Log_buffer {
  uint64_t magic;
  int64_t  capacity;
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

inline char* get_ringbuffer_loc()
{
  return (char*) OS::liveupdate_storage_area() - MRB_LOG_SIZE;
}

inline char* get_system_log_loc()
{
  return get_ringbuffer_loc() - sizeof(Log_buffer) - Log_buffer::PADDING;
}

void SystemLog::initialize()
{
  INFO("SystemLog", "Initializing System Log");

  auto& buffer = *((Log_buffer*) get_system_log_loc());
  mrb = buffer.get_mrb();

  // Correct magic means (hopefully) existing system log
  if(buffer.magic == Log_buffer::MAGIC)
  {
    auto* state = (int*)(&buffer.vla);

    new (mrb) MemoryRingBuffer(get_ringbuffer_loc(),
                    state[0], state[1], state[2], state[3]);

    INFO2("Restored @ %p (%i kB)", mrb->data(), mrb->capacity() / 1024);
  }
  // There isn't one, so we have to create
  else
  {
    new (mrb) MemoryRingBuffer(get_ringbuffer_loc(), MRB_LOG_SIZE);
    buffer.magic = Log_buffer::MAGIC;
    buffer.capacity = mrb->capacity();

    INFO2("Created @ %p (%i kB)", mrb->data(), mrb->capacity() / 1024);
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
