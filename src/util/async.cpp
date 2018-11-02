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
#include <common>
#include <memory>

inline unsigned roundup(unsigned n, unsigned div) {
  return (n + div - 1) / div;
}

void Async::upload_file(
    Disk          disk,
    const Dirent& ent,
    Stream*       stream,
    on_after_func callback,
    const size_t  CHUNK_SIZE)
{
  disk_transfer(disk, ent,
  [stream] (fs::buffer_t buffer,
            next_func    next) mutable
  {
    auto length = buffer->size();
    // temp
    stream->on_write(
      net::tcp::Connection::WriteCallback::make_packed(
      [length, next] (size_t n) {

        // if all data written, go to next chunk
        debug("<Async::upload_file> %u / %u\n", n, length);
        next(n == length);
      })
    );

    // write chunk to TCP connection
    stream->write(buffer);

  }, callback, CHUNK_SIZE);
}

void Async::disk_transfer(
    Disk          disk,
    const Dirent& ent,
    on_write_func write_func,
    on_after_func callback,
    const size_t  CHUNK_SIZE)
{
  typedef delegate<void(size_t)> next_func_t;
  auto next = std::make_shared<next_func_t> ();
  auto weak_next = std::weak_ptr<next_func_t>(next);

  *next = next_func_t::make_packed(
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
    disk->fs().read(
      ent,
      pos * CHUNK_SIZE,
      CHUNK_SIZE,
      fs::on_read_func::make_packed(
      [next, pos, write_func, callback] (
          fs::error_t  err,
          fs::buffer_t buffer)
      {
        debug("<Async> len=%lu\n", buffer->size());
        if (err) {
          printf("%s\n", err.to_string().c_str());
          callback(err, false);
          return;
        }

        // call write callback with data
        write_func(
          buffer,
          next_func::make_packed(
          [next, pos, callback] (bool good)
          {
            // if the write succeeded, call next
            if (LIKELY(good))
              (*next)(pos+1);
            else
              // otherwise, fail
              callback({fs::error_t::E_IO, "Write failed"}, false);
          })
        );
      })
    );
  });
  // start async loop
  (*next)(0);
}
