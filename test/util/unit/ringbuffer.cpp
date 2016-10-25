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
  RingBuffer rb(1);
  EXPECT(rb.capacity() == 1);
  
  EXPECT(rb.free_space() == 1);
  EXPECT(rb.used_space() == 0);
  
  EXPECT(rb.full()  == false);
  EXPECT(rb.empty() == true);
}
CASE("Adding bytes to ringbuffer")
{
  RingBuffer rb(2);
  
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
  RingBuffer rb(2);
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
CASE("We can discard data")
{
  RingBuffer rb(10);
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
