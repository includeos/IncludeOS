#include <hw/audio_device.hpp>
#include <hw/pci_device.hpp>

class AC97 : public hw::Audio_device
{
public:
  static const int NUM_DESC   = 32;
  static const int BUFFER_LEN = 0x1000;

  AC97(hw::PCI_Device&);

  std::string device_name() const override {
    return "ac97";
  }
  const char* driver_name() const noexcept override {
    return "Intel AC'97";
  }

  void     set_mixer(mixer_value_t, uint32_t) override;
  uint32_t get_mixer(mixer_value_t) override;

  void deactivate() override {}

  static std::unique_ptr<Audio_device> new_instance(hw::PCI_Device& d)
  { return std::make_unique<AC97>(d); }

private:
  uint8_t  read_mixer8  (uint16_t offset);
  uint16_t read_mixer16 (uint16_t offset);
  void     write_mixer8 (uint16_t offset, uint8_t value);
  void     write_mixer16(uint16_t offset, uint16_t value);
  uint8_t  read_nabm8  (uint16_t offset);
  uint16_t read_nabm16 (uint16_t offset);
  void     write_nabm8 (uint16_t offset, uint8_t value);
  void     write_nabm16(uint16_t offset, uint16_t value);
  void irq_handler();
  void enable_interrupts();
  void disable_interrupts();
  //
  uint8_t  m_irq;
  uint16_t m_mixer_io;
  uint16_t m_nabm_io;
  //
  struct buffer_t {
    std::array<uint8_t, 4096> buffer;
  };
  struct buffer_desc_t {
    uint32_t  buffer;
    uint32_t  ctlv;
  } __attribute__((packed));
  std::array<buffer_desc_t, NUM_DESC> m_desc = {};
  std::array<buffer_t, NUM_DESC> m_buffers;
  uint32_t m_current_idx = 0;
};
