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

#ifndef SERVICE_STATISTICS_HPP
#define SERVICE_STATISTICS_HPP

/**
 *
 */
struct Statistics {

  /**
   *
   */
  Statistics& bump_connection_count() noexcept
  { ++number_of_connections_; return *this; }

  /**
   *
   */
  Statistics& bump_data_received(const size_t n) noexcept
  { data_received_ += n; return *this; }

  /**
   *
   */
  Statistics& bump_data_sent(const size_t n) noexcept
  { data_sent_ += n; return *this; }

  /**
   *
   */
  Statistics& bump_request_received() noexcept
  { ++request_received_; return *this; }

  /**
   *
   */
  Statistics& bump_response_sent() noexcept
  { ++response_sent_; return *this; }

  /**
   *
   */
  Statistics& set_active_clients(const size_t n) noexcept
  { active_clients_ = n; return *this; }

  /**
   *
   */
  Statistics& set_memory_usage(const size_t n) noexcept
  { memory_usage_ = n; return *this; }

  /**
   *
   */
  template<typename Writer>
  void serialize(Writer& writer) const;

private:
  uint64_t data_received_         {0U};
  uint64_t data_sent_             {0U};

  uint64_t request_received_      {0U};
  uint64_t response_sent_         {0U};

  uint64_t number_of_connections_ {0U};

  uint64_t active_clients_        {0U};
  uint64_t memory_usage_          {0U};
}; //< struct Statistics

/**--v----------- Implementation Details -----------v--**/

template<typename Writer>
void Statistics::serialize(Writer& writer) const {
  writer.StartObject();

  writer.Key("DATA_RECV");
  writer.Uint64(data_received_);

  writer.Key("DATA_SENT");
  writer.Uint64(data_sent_);

  writer.Key("REQ_RECV");
  writer.Uint64(request_received_);

  writer.Key("RES_SENT");
  writer.Uint64(response_sent_);

  writer.Key("NO_CONN");
  writer.Uint64(number_of_connections_);

  writer.Key("ACTIVE_CONN");
  writer.Uint(active_clients_);

  writer.Key("MEM_USAGE");
  writer.Uint(memory_usage_);

  writer.EndObject();
}

/**--^----------- Implementation Details -----------^--**/

#endif //< SERVICE_STATISTICS_HPP
