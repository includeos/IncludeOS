
#include <common.cxx>
#include <virtio/virtio.hpp>

CASE("Virtio Queue enqueue")
{
  Virtio::Queue q("Test queue", 4096, 0, 0x1000);
  
  EXPECT(q.size() == 4096);

  uint8_t  value = 4;
  uint8_t  buffer[16];

  Virtio::Token token1 {{&value, sizeof(value)}, Virtio::Token::IN };
  Virtio::Token token2 {{buffer, sizeof(buffer)}, Virtio::Token::IN };

  std::array<Virtio::Token, 2> tokens {{ token1, token2 }};
  q.enqueue(tokens);
  // update avail idx
  q.kick();
}

CASE("Virtio Queue interrupts")
{
  Virtio::Queue q("Test queue", 4096, 0, 0x1000);
  q.enable_interrupts();
  EXPECT(q.interrupts_enabled());
  q.disable_interrupts();
  EXPECT(!q.interrupts_enabled());
}

CASE("Virtio Queue dequeue")
{
  Virtio::Queue q("Test queue", 4096, 0, 0x1000);
  
  EXPECT(q.size() == 4096);

  auto res = q.dequeue();
  EXPECT(res.size() == 0);
  EXPECT(res.data() == nullptr);
}
