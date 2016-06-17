// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#ifndef SNAKE_HPP
#define SNAKE_HPP

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
    reset();
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
    this->gameover = true;
    
    vga.set_cursor(32, 12);
    std::string gameover = "GAME OVER ! ! !";
    vga.write(gameover.c_str(), gameover.size());
    
    vga.set_cursor(32, 13);
    std::string finalscore = "SCORE: " + std::to_string(score);
    vga.write(finalscore.c_str(), finalscore.size());
  }
  bool is_gameover() const {
    return this->gameover;
  }
  
  void reset() {
    this->gameover = false;
    this->score    = 1;
    vga.clear();
    // create snake head
    parts.clear();
    parts.emplace_back(rand() % 80, rand() % 25, 0, 0);
    vga.put('@', 2, parts[0].get_x(), parts[0].get_y());
    // place the first food
    place_food();
    
    hw::PIT::on_timeout(0.2,
    [this] {
      this->integrate();
    });
  }
  
private:
  ConsoleVGA& vga;
  bool     gameover;
  int8_t   dirx = 1;
  int8_t   diry = 0;
  uint16_t score;
  std::vector<Part> parts;
};

#endif
