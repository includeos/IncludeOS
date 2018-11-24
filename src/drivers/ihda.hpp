#include <hw/audio_device.hpp>
#include <hw/pci_device.hpp>

class IHDA : public hw::Audio_device
{
public:
  static const int NUM_CORB   = 16;
  static const int BUFFER_LEN = 0x1000;

  IHDA(hw::PCI_Device&);

  std::string device_name() const override {
    return "ihda";
  }
  const char* driver_name() const noexcept override {
    return "Intel High-Definition Audio";
  }

  void     set_mixer(mixer_value_t, uint32_t) override;
  uint32_t get_mixer(mixer_value_t) override;

  void deactivate() override {}

  static std::unique_ptr<Audio_device> new_instance(hw::PCI_Device& d)
  { return std::make_unique<IHDA>(d); }

private:
  template <typename T> T read(uint32_t);
  template <typename T> void write(uint32_t, T);
  void reset();
  void detect_codecs();
  void event_handler();

  struct widget_t
  {
    const char* name() const noexcept;
    uint8_t nid() const noexcept { return this->m_node_id; }
    uint8_t type() const noexcept { return (this->m_caps >> 20) & 0xF; }

    uint32_t m_node_id;
    uint32_t m_type;
    uint32_t m_conn_listen_len;
    uint32_t m_caps;
    uint32_t m_first_conn_list_entry;
  };
  struct afg_t {
    uint32_t node_id;
    uint32_t type;
    uint32_t start_node;
    uint32_t node_count;
    std::vector<widget_t> widgets;
  };
  struct codec_t {
    uint8_t  idx = 0;
    uint32_t start_node;
    std::vector<afg_t> afgs;
  };
  struct response_t {
    const uint32_t resp;
    const uint32_t rext;
  };

  union verb_t {
    struct {
      uint32_t verb : 20;
      uint32_t nid  : 7;
      uint32_t indirect_nid : 1;
      uint32_t cad  : 4;
    };
    uint32_t whole = 0;
  };
  struct alignas(128) corb_t {
    verb_t* entries = nullptr;
    uint32_t reg = 0;
  };

  void setup_ring(corb_t&, bool corb);
  void start_dma(corb_t&);
  void stop_dma(corb_t&);
  void blocking_write(corb_t&, verb_t&);
  response_t blocking_read(corb_t& ring);
  response_t corb_read(verb_t&);
  response_t immediate_read(verb_t&);
  // GetParam function
  response_t get(uint8_t nid, uint8_t cad, uint8_t param);
  response_t cmd(uint8_t nid, uint8_t cad, uint32_t cmd);

  uintptr_t m_hdbar = 0;
  uint8_t   m_irq;
  corb_t m_corb;
  corb_t m_rirb;
  std::vector<codec_t> m_codecs;

  static_assert(sizeof(verb_t) == 4, "Entries are 4 bytes each");
};
