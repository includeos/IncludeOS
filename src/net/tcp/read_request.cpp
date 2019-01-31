// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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

#include <net/tcp/read_request.hpp>

namespace net {
namespace tcp {

  Read_request::Read_request(seq_t start, size_t min, size_t max, Alloc&& alloc)
    : alloc{alloc}
  {
    buffers.push_back(std::make_unique<Read_buffer>(start, min, max, alloc));
  }

  size_t Read_request::insert(seq_t seq, const uint8_t* data, size_t n, bool psh)
  {
    Expects(not buffers.empty());

    //printf("insert: seq=%u len=%lu\n", seq, n);
    size_t recv{0};
    while(n)
    {
      auto* buf = get_buffer(seq);

      if(UNLIKELY(buf == nullptr)) {
        //printf("no buffer found\n");
        break;
      }

      //printf("got buffer start=%u end=%u missing=%lu\n",
      //  buf->start_seq(), buf->end_seq(), buf->missing());

      auto read = buf->insert(seq, data + recv, n, psh);

      n -= read; // subtract amount of data left to insert
      recv += read; // add to the total recieved
      seq += read; // advance the sequence number

      // flush this (and other buffers) if ready
      // (and also being the one in the front)
      while(buf->is_ready() and buf == buffers.front().get())
      {
        const auto rem = buf->capacity() - buf->size();
        const auto end_seq = buf->end_seq(); // store end_seq if reseted in callback

        if (on_read_callback != nullptr) {
          on_read_callback(buf->buffer());
        } else {
          // Ready buffer for read_next
          complete_buffers.push_back(buf->buffer());
        }

        // this is the only one, so we can reuse it
        if(buffers.size() == 1)
        {
          // Trick to make SACK work. If a lot of data was cleared up
          // it means the local sequence number is much farther behind
          // the real one
          seq = end_seq - rem;
          buf->reset(seq);
          //printf("size=1, reset rem=%u start=%u end=%u\n",
          //  rem, buf->start_seq(), buf->end_seq());
          break;
        }
        // to make it simple, just get rid of it and have the
        // algo create a new one if needed
        // TODO: reuse it by putting in in the back
        // (need to check if there is room for more, and
        // maybe it isnt even necessary..)
        else
        {
          //printf("size=%zu rem=%zu\n", buffers.size(), rem);
          // if there are other buffers following this one,
          // and the buffer wasnt full, fill the small gap
          if(UNLIKELY(rem != 0))
          {
            buf->reset(buf->end_seq()-(uint32_t)rem, rem);
            Ensures(buf->end_seq() == buffers.at(1)->start_seq());
          }
          else
          {
            //printf("finished, pop front start=%u end=%u\n",
            //  buf->start_seq(), buf->end_seq());
            buffers.pop_front();
            buf = buffers.front().get();
          }
        }

      } // < while(buf->is_ready() and buf == buffers.front().get())

    } // < while(n)

    signal_data();

    Ensures(not buffers.empty());
    return recv;
  }

  Read_buffer* Read_request::get_buffer(const seq_t seq)
  {
    // There is room in a existing one
    for(auto& ptr : buffers)
    {
      if(ptr->fits(seq) > 0)
        return ptr.get();
    }

    //printf("seq=%u do not fit in any current buffer\n", seq);
    // We can still create a new one
    if(buffers.size() < buffer_limit)
    {
      // current cap
      const auto& cur_back = buffers.back();
      //printf("current back, start=%u end=%u sz=%u\n",
      //  cur_back->start_seq(), cur_back->end_seq(), cur_back->size());

      // TODO: if the gap is bigger than 1 buffer
      // we probably need to create multiple buffers,
      // ... or just decide we only support gaps of 1 buffer size.
      buffers.push_back(
        std::make_unique<Read_buffer>(cur_back->end_seq(), cur_back->capacity(), cur_back->capacity(), alloc));

      auto& back = buffers.back();
      //printf("new buffer added start=%u end=%u, fits(%lu)=%lu\n",
      //  back->start_seq(), back->end_seq(), seq, back->fits(seq));

      if(back->fits(seq) > 0)
        return back.get();
    }
    //printf("did not fit\n");

    return nullptr;
  }

  size_t Read_request::fits(const seq_t seq) const
  {
    auto len = 0;
    // There is room in a existing one
    for(auto& ptr : buffers)
    {
      len += ptr->fits(seq);
      /*if(ptr->fits(seq) > 0)
        return ptr->fits(seq);*/
    }

    // Possible to create one (or more) to support this?
    if(buffers.size() < buffer_limit)
    {
      auto& back = buffers.back();
      const auto rel = seq - back->end_seq();
      const auto cap = back->capacity();

      if(rel < cap)
        len += (cap - rel);
    }

    return len;
  }

  size_t Read_request::size() const
  {
    size_t bytes = 0;

    for(auto& ptr : buffers)
      bytes += ptr->size();

    return bytes;
  }

  void Read_request::set_start(seq_t seq)
  {
    for(auto& ptr : buffers)
    {
      Expects(ptr->size() == 0 && "Cannot change start sequence when there already is data");
      ptr->set_start(seq);
      seq += ptr->capacity();
      Ensures(seq == ptr->end_seq());
    }
  }

  void Read_request::signal_data() {

    if (not complete_buffers.empty()) {
      if (on_data_callback != nullptr){
        on_data_callback();
        if (not complete_buffers.empty()) {
          // FIXME: Make sure this event gets re-triggered
          // For now the user will have to make sure to re-read later if they couldn't
        }
      } else if (on_read_callback != nullptr) {
        for (auto buf : complete_buffers) {
          // Pop each time, in case callback leads to another call here.
          complete_buffers.pop_front();
          on_read_callback(buf);
        }
      }
    }
  }

  size_t Read_request::next_size() {
    if (not complete_buffers.empty()) {
      return complete_buffers.front()->size();
    }
    return 0;
  }

  buffer_t Read_request::read_next() {
    if (UNLIKELY(complete_buffers.empty()))
        return nullptr;
    auto buf = std::move(complete_buffers.front());
    complete_buffers.pop_front();
    return buf;
  }

  void Read_request::reset(const seq_t seq)
  {
    Expects(not buffers.empty());

    auto it = buffers.begin();

    // get the first buffer
    auto* buf = it->get();

    // okay, so here's the deal.
    // there might be data in the read_buffer (due to liveupdate restore)
    // so we need to be able to set a callback, then have reset called,
    // to flush the data to the user.

    // if noone is using the buffer right now, (stupid yes)
    // AND it contains data without any holes,
    // return it to the user
    if (buf->has_unhandled_data())
    {
      complete_buffers.push_back(buf->buffer());
    }

    signal_data();

    // reset the first buffer
    buf->reset(seq);
    // throw the others away
    buffers.erase(++it, buffers.end());

    Ensures(buffers.size() == 1);
  }

}
}
