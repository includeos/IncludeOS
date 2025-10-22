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
#include <util/ringbuffer.hpp>

CASE("A new ringbuffer is empty")
{
  HeapRingBuffer rb(1);
  EXPECT(rb.capacity() == 1);

  EXPECT(rb.free_space() == 1);
  EXPECT(rb.used_space() == 0);

  EXPECT(rb.full()  == false);
  EXPECT(rb.empty() == true);
}
CASE("Adding bytes to ringbuffer")
{
  HeapRingBuffer rb(2);

  int written = rb.write("1", 1);
  EXPECT(written == 1);

  EXPECT(rb.free_space() == 1);
  EXPECT(rb.used_space() == 1);

  EXPECT(rb.full()  == false);
  EXPECT(rb.empty() == false);

  written = rb.write("2", 1);
  EXPECT(written == 1);

  EXPECT(rb.free_space() == 0);
  EXPECT(rb.used_space() == 2);

  EXPECT(rb.full()  == true);
  EXPECT(rb.empty() == false);

  EXPECT(rb.size() == rb.used_space());
}
CASE("Reading bytes to ringbuffer")
{
  HeapRingBuffer rb(2);
  int written;

  written = rb.write("1", 1);
  EXPECT(written == 1);
  written = rb.write("2", 1);
  EXPECT(written == 1);

  char buffer[1];
  int read;

  read = rb.read(buffer, sizeof(buffer));
  EXPECT(read == 1);
  EXPECT(buffer[0] == '1');

  EXPECT(rb.used_space() == 1);
  EXPECT(rb.free_space() == 1);

  read = rb.read(buffer, sizeof(buffer));
  EXPECT(read == 1);
  EXPECT(buffer[0] == '2');

  EXPECT(rb.used_space() == 0);
  EXPECT(rb.free_space() == 2);

  EXPECT(rb.full()  == false);
  EXPECT(rb.empty() == true);

  EXPECT(rb.size() == rb.used_space());
}
CASE("Read Pt.2")
{
  const char* buffer = "ABCDEFGH";
  const int len = strlen(buffer);

  HeapRingBuffer rb(len);
  // advance one position internally
  rb.write("A", 1);
  rb.discard(1);
  EXPECT(rb.empty());
  // write known buffer
  rb.write(buffer, len);
  // read it back
  char readback[10];
  EXPECT(rb.read(readback, sizeof(readback)) == len);
  EXPECT(memcmp(readback, buffer, len) == 0);
}
CASE("We can discard data")
{
  HeapRingBuffer rb(10);
  int written;

  written = rb.write("12345", 5);
  EXPECT(written == 5);
  written = rb.write("67890", 5);
  EXPECT(written == 5);

  // discard half the data
  rb.discard(5);

  EXPECT(rb.used_space() == 5);
  EXPECT(rb.free_space() == 5);

  EXPECT(rb.full()  == false);
  EXPECT(rb.empty() == false);

  // discard the rest
  rb.discard(5);

  EXPECT(rb.used_space() == 0);
  EXPECT(rb.free_space() == rb.capacity());

  EXPECT(rb.full()  == false);
  EXPECT(rb.empty() == true);
}
CASE("Discard pt.2")
{
  HeapRingBuffer rb(10);
  int written;
  written = rb.write("1234567890", 10);
  EXPECT(written == 10);

  // discard 1, read 1, compare
  char data[1];

  rb.discard(1);
  rb.read(data, sizeof(data));
  EXPECT(data[0] == '2');
  rb.write("12", 2);

  rb.discard(1);
  rb.read(data, sizeof(data));
  EXPECT(data[0] == '4');
  rb.write("34", 2);

  rb.discard(1);
  rb.read(data, sizeof(data));
  EXPECT(data[0] == '6');
  rb.write("56", 2);

  rb.discard(1);
  rb.read(data, sizeof(data));
  EXPECT(data[0] == '8');
  rb.write("78", 2);

  rb.discard(1);
  rb.read(data, sizeof(data));
  EXPECT(data[0] == '0');
  rb.write("90", 2);

  EXPECT(rb.size() == 10);
  char all_data[10];
  rb.read(all_data, sizeof(all_data));
  EXPECT(memcmp(all_data, "1234567890", 10) == 0);
}

CASE("Test sequentialize Pt.1")
{
  {
    HeapRingBuffer rb(1);
    rb.write("A", 1);
    EXPECT(strncmp(rb.sequentialize(), "A", 1) == 0);
  }
  {
    HeapRingBuffer rb(2);
    rb.write("AB", 2);
    EXPECT(strncmp(rb.sequentialize(), "AB", 2) == 0);
  }
  {
    HeapRingBuffer rb(2);
    rb.write("A", 1);
    rb.discard(1);
    rb.write("AB", 2);
    EXPECT(strncmp(rb.sequentialize(), "AB", 2) == 0);
  }
}
CASE("Test sequentialize Pt.2")
{
  const char* buffer = "ABCDEFGH";
  const int len = strlen(buffer);
  HeapRingBuffer rb(len);

  for (int i = 0; i < len; i++)
  {
    // advance one position internally
    rb.write("A", 1);
    rb.discard(1);
    EXPECT(rb.empty());
    // write known buffer
    rb.write(buffer, len);
    // verify its sequential
    const char* result = rb.sequentialize();
    EXPECT(memcmp(result, buffer, len) == 0);
    rb.discard(len);
  }
}
