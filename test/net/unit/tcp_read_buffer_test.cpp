// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <net/tcp/read_buffer.hpp>

CASE("Filling a hole")
{
  using namespace net::tcp;
  const seq_t SEQ_START = 322;
  seq_t SEQ = SEQ_START;
  const size_t BUFSZ = 128;

  Read_buffer buf{BUFSZ, SEQ};

  EXPECT(buf.size() == 0);
  EXPECT(buf.capacity() == BUFSZ);
  EXPECT(buf.missing() == 0);
  EXPECT(not buf.is_ready());
  EXPECT(not buf.at_end());

  using namespace std::string_literals;

  std::string str1, str2, str3;
  str1 = "This is a test"s;
  str2 = "And this is a hole to be filled"s;
  str3 = "And this is the end"s;

  size_t written = 0;

  written = buf.insert(SEQ, (uint8_t*)str1.data(), str1.size());
  EXPECT(written == str1.size());
  SEQ += str1.size();

  EXPECT(buf.missing() == 0);
  EXPECT(buf.size() == SEQ - SEQ_START);
  EXPECT(not buf.is_ready());

  const auto seq_hole = SEQ;
  SEQ += str2.size();

  written = buf.insert(SEQ, (uint8_t*)str3.data(), str3.size(), true);
  EXPECT(written == str3.size());
  SEQ += str3.size();

  EXPECT(buf.missing() == str2.size());
  EXPECT(buf.size() == SEQ - SEQ_START);
  EXPECT(not buf.is_ready());


  written = buf.insert(seq_hole, (uint8_t*)str2.data(), str2.size());
  EXPECT(written == str2.size());

  EXPECT(buf.missing() == 0);
  EXPECT(buf.size() == SEQ - SEQ_START);
  EXPECT(buf.is_ready());

  std::string compare = str1+str2+str3;

  EXPECT(std::memcmp(buf.buffer()->data(), compare.data(), compare.size()) == 0);

  EXPECT(not buf.at_end());

  EXPECT(SEQ - SEQ_START == compare.size());
}

CASE("Filling the buffer")
{
  using namespace net::tcp;
  const seq_t SEQ_START = 322;
  seq_t SEQ = SEQ_START;
  const size_t BUFSZ = 128;

  Read_buffer buf{BUFSZ, SEQ};

  using namespace std::string_literals;

  std::string str1, str2, str3, str4;
  str1 = "This is a test"s;
  str2 = "And this is a hole to be filled"s;
  str3 = "And this is the end"s;
  str4 = "This string will exceed the limit of the buffer due to the length"s;

  size_t written = 0;

  SEQ += buf.insert(SEQ, (uint8_t*)str1.data(), str1.size());

  SEQ += buf.insert(SEQ, (uint8_t*)str2.data(), str2.size());

  SEQ += buf.insert(SEQ, (uint8_t*)str3.data(), str3.size());

  written = buf.insert(SEQ, (uint8_t*)str4.data(), str4.size());
  SEQ += written;

  auto rem = str4.size() - written;

  EXPECT(rem > 0);
  EXPECT(buf.at_end());
  EXPECT(buf.is_ready());

  EXPECT(SEQ - SEQ_START == buf.capacity());
}

CASE("Reseting the buffer")
{
  using namespace net::tcp;
  const seq_t SEQ_START = 322;
  seq_t SEQ = SEQ_START;
  const size_t BUFSZ = 128;

  Read_buffer buf{BUFSZ, SEQ};

  using namespace std::string_literals;

  std::string str1;
  str1 = "This is a test"s;

  size_t written = 0;

  SEQ += buf.insert(SEQ, (uint8_t*)str1.data(), str1.size(), true);

  EXPECT(buf.size() == str1.size());
  EXPECT(buf.is_ready());

  auto buffer = buf.buffer();
  EXPECT(buffer.use_count() == 2);

  buf.reset(SEQ);

  EXPECT(buf.capacity() == BUFSZ);
  EXPECT(buf.size() == 0);
  EXPECT(buf.missing() == 0);
  EXPECT(not buf.is_ready());
  EXPECT(not buf.at_end());

  // not unique means new buffer
  EXPECT(buffer.unique());
  EXPECT(buf.buffer() != buffer);

  // no copy means same buffer
  auto* ptr = buf.buffer().get();
  buf.reset(SEQ);
  EXPECT(buf.buffer().get() == ptr);

  // increasing the cap means same buffer
  ptr = buf.buffer().get();
  auto* data = buf.buffer()->data();
  buf.reset(SEQ, BUFSZ*2);
  EXPECT(buf.buffer().get() == ptr);
  // but not same data
  EXPECT(buf.buffer()->data() != data);

  // decreasing the cap also means new data
  data = buf.buffer()->data();
  buf.reset(SEQ, BUFSZ/2);
  EXPECT(buf.buffer()->data() != data);
}
