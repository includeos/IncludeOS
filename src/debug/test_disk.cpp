#include <os>
#include <cassert>
const char* service_name__ = "...";

#include <fs/disk.hpp>
fs::Disk_ptr disk;

#include <net/inet4>
std::unique_ptr<net::Inet4<VirtioNet> > inet;

#include <vga>
ConsoleVGA vga;

void list_partitions(decltype(disk));

void print_shit(fs::Disk_ptr disk)
{
  static size_t ints = 0;
  printf("PIT Interrupt #%u\n", ++ints);
  
  disk->dev().read(0,
  [disk] (fs::buffer_t buffer)
  {
    assert(!!buffer);
    hw::PIT::on_timeout(0.25, [disk] { print_shit(disk); });
  });
}

#include <hw/apic.hpp>
#include <hw/kbm.hpp>

#include <string>

class Snake
{
public:
  static const char WHITESPACE = ' ';
  
  enum {
    UP,
    DOWN,
    RIGHT,
    LEFT
  };
  
  Snake(ConsoleVGA& con)
    : vga(con)
  {
    vga.clear();
    // create snake head
    parts.emplace_back(rand() % 80, rand() % 25, 0, 0);
    vga.put('@', 2, parts[0].get_x(), parts[0].get_y());
    // place the first food
    place_food();
    
    hw::PIT::on_timeout(0.2,
    [this] {
      this->integrate();
    });
  }
  
  // 80x25 = 2000
  // 11 bits = 2048
  //  2 bits = direction
  //  3 bits = type
  struct Part {
    uint16_t data;
    
    Part(uint16_t x, uint16_t y, uint16_t dir, uint16_t typ)
      : data(y * 80 + x)
    {
      set_dir(dir);
      set_type(typ);
    }
    
    void set_pos(uint16_t x, uint16_t y)
    {
      data &= ~0x7FF;
      data |= y * 80 + x;
    }
    uint16_t get_x() const noexcept
    {
      return (data & 0x7FF) % 80;
    }
    uint16_t get_y() const noexcept
    {
      return (data & 0x7FF) / 80;
    }
    
    void set_dir(uint16_t dir)
    {
      data &= 0xE7FF;
      data |= (dir & 3) << 11;
    }
    uint16_t get_dir() const noexcept
    {
      return (data >> 11) & 3;
    }
    
    void set_type(uint16_t typ)
    {
      data &= 0x1FFF;
      data |= typ << 13;
    }
    uint16_t get_type() const noexcept
    {
      return data >> 13;
    }
    
    char get_symbol() const noexcept
    {
      switch (get_type()) {
      case 0:
        return '@';
      case 1:
        return '*';
      case 2:
        return '.';
      }
      return '?';
    }
    
  };
  
  void set_dir(uint16_t dir)
  {
    switch (dir) {
    case UP:
      if (diry == 1) return;
      dirx = 0;
      diry = -1;
      return;
    case DOWN:
      if (diry == -1) return;
      dirx = 0;
      diry = 1;
      return;
    case RIGHT:
      if (dirx == -1) return;
      dirx = 1;
      diry = 0;
      return;
    case LEFT:
      if (dirx == 1) return;
      dirx = -1;
      diry = 0;
      return;
    }
    dirx = 0;
    diry = 0;
  }
  void integrate()
  {
    auto& head = parts[0];
    int16_t newx = (head.get_x() + this->dirx) % 80;
    if (newx < 0) newx = 79;
    int16_t newy = (head.get_y() + this->diry) % 25;
    if (newy < 0) newy = 24;
    
    bool longer = false;
    auto ent = vga.get(newx, newy);
    if (is_food(ent & 0xff)) {
      /// food ///
      longer = true;
      score++;
    }
    else if (!is_whitespace(ent & 0xff)) {
      /// game over ///
      game_over();
      return;
      /// game over ///
    }
    
    auto old_last = parts[parts.size()-1];
    
    // move rest
    for (size_t p = parts.size()-1; p > 0; p--)
    {
      parts[p].set_pos(parts[p-1].get_x(), parts[p-1].get_y());
      draw_part(parts[p]);
    }
    // move head
    head.set_pos(newx, newy);
    draw_part(head);
    
    // erase last part
    if (!longer) {
      vga.put(WHITESPACE, 0, old_last.get_x(), old_last.get_y());
    }
    else {
      old_last.set_type(1);
      draw_part(old_last);
      parts.push_back(old_last);
      // place more food
      place_food(); place_food();
      // and some dangerous stuff
      place_mine(); place_mine();
    }
    
    hw::PIT::on_timeout(0.1,
    [this] {
      integrate();
    });
  }
  
  void place_token(const char tk, const uint8_t color)
  {
    while (true) {
      int x = rand() % 80;
      int y = rand() % 25;
      // ignore non-whitespaces
      if (!is_whitespace(vga.get(x, y) & 0xff)) continue;
      // place food
      vga.put(tk, color, x, y);
      break;
    }
  }
  void place_food() {
    place_token('#', 2);
  }
  bool is_food(const char c) const
  {
    return c == '#';
  }
  void place_mine() {
    place_token('X', 4);
  }
  
  void draw_part(const Part& part)
  {
    vga.put(part.get_symbol(), 1, part.get_x(), part.get_y());
  }
  
  bool is_whitespace(const char c) const
  {
    return c == ' ';
  }
  void game_over() {
    /// game over ///
    vga.clear();
    
    vga.set_cursor(32, 12);
    std::string gameover = "GAME OVER ! ! !";
    vga.write(gameover.c_str(), gameover.size());
    
    vga.set_cursor(32, 13);
    std::string finalscore = "SCORE: " + std::to_string(score);
    vga.write(finalscore.c_str(), finalscore.size());
  }
  
private:
  ConsoleVGA& vga;
  int8_t   dirx = 1;
  int8_t   diry = 0;
  uint16_t score = 1;
  std::vector<Part> parts;
};

void begin_snake()
{
  static Snake snake(vga);
  
  hw::KBM::set_virtualkey_handler(
  [] (int key) {
    
    if (key == hw::KBM::VK_RIGHT) {
      snake.set_dir(Snake::RIGHT);
    }
    if (key == hw::KBM::VK_LEFT) {
      snake.set_dir(Snake::LEFT);
    }
    if (key == hw::KBM::VK_UP) {
      snake.set_dir(Snake::UP);
    }
    if (key == hw::KBM::VK_DOWN) {
      snake.set_dir(Snake::DOWN);
    }
    if (key == hw::KBM::VK_SPACE) {
      //
    }
    
  });
  
}

void Service::start()
{
  OS::set_rsprint(
  [] (const char* data, size_t len)
  {
    vga.write(data, len);
  });
  
  hw::KBM::init();
  // we have to start snake later to avoid late text output
  hw::PIT::on_timeout(0.2, [] { begin_snake(); });
  
  
  // instantiate memdisk with FAT filesystem
  auto& device = hw::Dev::disk<1, VirtioBlk>();
  disk = std::make_shared<fs::Disk> (device);
  assert(disk);
  
  // boilerplate
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0, 0.15);
  inet->network_config(
    { 10,0,0,42 },      // IP
    { 255,255,255,0 },  // Netmask
    { 10,0,0,1 },       // Gateway
    { 8,8,8,8 } );      // DNS
  
  // if the disk is empty, we can't mount a filesystem anyways
  if (disk->empty()) panic("Oops! The disk is empty!\n");
  
  // 1. create alot of separate jobs
  for (int i = 0; i < 256; i++)
  device.read(0,
  [i] (fs::buffer_t buffer)
  {
    if (!buffer)
      printf("buffer %d is not null: %d\n", i, !!buffer);
    assert(buffer);
  });
  // 2. create alot of sequential jobs of 1024 sectors each
  // note: if we queue more than this we will run out of RAM
  static int bufcounter = 0;
  
  for (int i = 0; i < 256; i++)
  device.read(0, 22,
  [i] (fs::buffer_t buffer)
  {
    assert(buffer);
    bufcounter++;
    
    if (bufcounter == 256)
      printf("Success: All big buffers accounted for\n");
  });
  
  // list extended partitions
  list_partitions(disk);

  // mount first valid partition (auto-detect and mount)
  disk->mount(
  [] (fs::error_t err) {
    if (err) {
      printf("Error: %s\n", err.to_string().c_str());
      return;
    }
    printf("Mounted filesystem\n");
    
    // read something that doesn't exist
    disk->fs().stat("/fefe/fefe",
    [] (fs::error_t err, const fs::Dirent&) {
      printf("Error: %s\n", err.to_string().c_str());
    });
    
    // async ls
    disk->fs().ls("/",
    [] (fs::error_t err, auto ents) {
      if (err) {
        printf("Could not list '/' directory\n");
        return;
      }
      printf("Listed filesystem root\n");
      
      // go through directory entries
      for (auto& e : *ents) {
        printf("%s: %s\t of size %llu bytes (CL: %llu)\n",
               e.type_string().c_str(), e.name().c_str(), e.size(), e.block);
        
        if (e.is_file()) {
          printf("*** Read file  %s\n", e.name().c_str());
          disk->fs().read(e, 0, e.size(),
          [e] (fs::error_t err, fs::buffer_t buffer, size_t len) {
            if (err) {
              printf("Failed to read file %s!\n",
                     e.name().c_str());
              return;
            }
            
            std::string contents((const char*) buffer.get(), len);
            printf("[%s contents]:\n%s\nEOF\n\n",
                   e.name().c_str(), contents.c_str());
          });
        }
      }
    }); // ls
  }); // disk->auto_detect()
  
  return;
  static int job = 0;
  
  for (int i = 0; i < 10; i++)
  hw::APIC::start_task(
  [] (int cpu_id) {
    (void) cpu_id;
    __sync_fetch_and_add(&job, 1);
  }, [] {
    printf("All jobs are done now, job = %d\n", job);
  });
  
  printf("*** TEST SERVICE STARTED *** \n");
}

void list_partitions(decltype(disk) disk)
{
  disk->partitions(
  [] (fs::error_t err, auto& parts) {
    if (err) {
      printf("Failed to retrieve volumes on disk\n");
      return;
    }
    
    for (auto& part : parts)
      printf("[Partition]  '%s' at LBA %u\n",
             part.name().c_str(), part.lba());
  });
}
