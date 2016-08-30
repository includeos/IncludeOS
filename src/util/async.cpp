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

#include <async>

#include <memory>

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

inline unsigned roundup(unsigned n, unsigned div) {
  return (n + div - 1) / div;
}

void Async::upload_file(
    Disk          disk,
    const Dirent& ent,
    Connection    conn,
    on_after_func callback,
    const size_t  CHUNK_SIZE)
{
  disk_transfer(disk, ent,
  [conn] (fs::buffer_t buffer,
          size_t       length,
          next_func    next)
  {
    // write chunk to TCP connection
    conn->write(buffer, length,
    [length, next] (size_t n) {

      // if all data written, go to next chunk
      debug("<Async::upload_file> %u / %u\n", n, length);
      next(n == length);

    });

  }, callback, CHUNK_SIZE);
}

void Async::disk_transfer(
    Disk          disk,
    const Dirent& ent,
    on_write_func write_func,
    on_after_func callback,
    const size_t  CHUNK_SIZE)
{
  typedef std::function<void(size_t)> next_func_t;
  auto next = std::make_shared<next_func_t> ();
  auto weak_next = std::weak_ptr<next_func_t>(next);

  *next =
  [weak_next, disk, ent, write_func, callback, CHUNK_SIZE] (size_t pos) {

    // number of write calls necessary
    const size_t writes = roundup(ent.size(), CHUNK_SIZE);

    // done condition
    if (pos >= writes) {
      callback(fs::no_error, true);
      return;
    }
    auto next = weak_next.lock();
    // read chunk from file
    disk->fs().read(ent, pos * CHUNK_SIZE, CHUNK_SIZE,
    [next, pos, write_func, callback, CHUNK_SIZE] (
        fs::error_t  err,
        fs::buffer_t buffer,
        uint64_t     length)
    {
      debug("<Async> len=%llu\n",length);
      if (err) {
        printf("%s\n", err.to_string().c_str());
        callback(err, false);
        return;
      }

      // call write callback with data
      write_func(buffer, length,
      [next, pos, callback] (bool good) {
        // if the write succeeded, call next
        if (likely(good))
          (*next)(pos+1);
        else
          // otherwise, fail
          callback({fs::error_t::E_IO, "Write failed"}, false);
      });
    });

  };
  // start async loop
  (*next)(0);
}
