#include <vga>
#include <arch.hpp>
#include <ringbuffer>
#include <hw/ps2.hpp>

static const int SUGGEST_READ_SIZE = 800;

static RingBuffer emerg_buffer(1 << 17);
static bool procedure_entered = false;
static const char* read_position = nullptr;
static const char* read_minpos = nullptr;
static const char* read_maxpos = nullptr;
static size_t read_size = 0;

static void emergency_logging(const char* data, int length)
{
  int free = emerg_buffer.free_space();
  if (free < length) {
    emerg_buffer.discard(length - free);
  }
  emerg_buffer.write(data, length);
}

extern "C" void (*current_eoi_mechanism)();
extern "C" void (*current_intr_handler)();
extern "C"
void keyboard_emergency_handler()
{
  auto& vga = TextmodeVGA::get();
  vga.clear();

  using namespace hw;
  int key = KBM::get_kbd_vkey();
  switch (key)
  {
  case KBM::VK_UP:
      read_position = std::max(read_minpos, read_position - 100);
      vga.write("UP!\n", 4);
      break;
  case KBM::VK_DOWN:
      read_position = std::min(read_maxpos, read_position + 100);
      vga.write("DN!\n", 4);
      break;
  }
  // update
  vga.write(read_position, read_size);

  current_eoi_mechanism();
}

extern "C"
void panic_perform_inspection_procedure()
{
  if (procedure_entered) return;
  procedure_entered = true;

  const char* EMERG_INFO =
      "\n>>> VGA EMERGENCY MODE ENTERED! <<<\n"
      "Use arrow keys to navigate logbuffer\n";

  emerg_buffer.write(EMERG_INFO, strlen(EMERG_INFO));

  // disable all IRQs
  for (uint8_t i = 0; i < 128; i++)
    __arch_unsubscribe_irq(i);

  // enable interrupts again
  asm("sti");

  // enable keyboard and mouse
  hw::KBM::init();

  // make buffer sequential
  auto* buffer = emerg_buffer.sequentialize();

  // set position and size
  read_size = std::min(emerg_buffer.size(), SUGGEST_READ_SIZE);
  read_minpos = buffer;
  read_maxpos = buffer + emerg_buffer.size() - read_size;

  read_position = read_maxpos;

  // print initial buffer
  auto& vga = TextmodeVGA::get();
  vga.clear();
  vga.write(read_position, read_size);

  const uint8_t kbd_irq = hw::KBM::get_kbd_irq();
  __arch_subscribe_irq(kbd_irq);
  current_intr_handler = keyboard_emergency_handler;
}

static __attribute__((constructor))
void hest()
{
  OS::add_stdout(emergency_logging);
}
