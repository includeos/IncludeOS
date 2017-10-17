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

#include <cmath> // rand()
#include <sstream>

#include <os>
#include <hw/devices.hpp>
#include <kernel/events.hpp>
#include <drivers/usernet.hpp>
#include <net/inet4>


using namespace std::chrono;

inline UserNet& create_nic(){
  // the IncludeOS packet communicator
  auto* usernet = new UserNet(1504);
  // register driver for superstack
  auto driver = std::unique_ptr<hw::Nic> (usernet);
  hw::Devices::register_device<hw::Nic> (std::move(driver));

  return *usernet;
}

const int chunksize = 1024 * 1024;
const int num_chunks = 512;



class Async_device {
public:
  using delg = delegate<void(net::Packet_ptr)>;

  void transmit(net::Packet_ptr pckt) {

  };

  void receive(net::Packet_ptr pckt) {
    queue.push_back(std::move(pckt));
    Events::get().trigger_event(event_id);
  };

  Async_device (delegate<void(net::Packet_ptr)> dr)
    : driver_receive(dr)
  {
    event_id = Events::get().subscribe([this]{
        while(! queue.empty()) {
          this->driver_receive(std::move(queue.front()));
          queue.pop_front();
        }
      });
  }

private:
  delegate<void(net::Packet_ptr)> driver_receive;
  int event_id = 0;
  std::deque<net::Packet_ptr> queue;
};


std::unique_ptr<Async_device> dev1;
std::unique_ptr<Async_device> dev2;

using namespace std::chrono;
milliseconds time_start;

inline auto now() {
  return duration_cast< milliseconds >(system_clock::now().time_since_epoch());
}



void Service::start()
{


  // Create some pure userspace Nic's
  auto& nic1 = create_nic();
  auto& nic2 = create_nic();

  delegate<void(net::Packet_ptr)> delg1 {&nic1, &UserNet::receive};
  delegate<void(net::Packet_ptr)> delg2 {&nic2, &UserNet::receive};

  dev1 = std::make_unique<Async_device>(delg1);
  dev2 = std::make_unique<Async_device>(delg2);

  // Connect them with a wire
  nic1.set_transmit_forward({dev2.get(), &Async_device::receive});
  nic2.set_transmit_forward({dev1.get(), &Async_device::receive});

  // Create IP stacks on top of the nic's and configure them
  auto& inet_server = net::Super_stack::get<net::IP4>(0);
  auto& inet_client = net::Super_stack::get<net::IP4>(1);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,1});


  // Set up a TCP server on port 80
  auto& server = inet_server.tcp().listen(80);

  auto buf =  net::tcp::construct_buffer(chunksize);

  // Add a TCP connection handler
  server.on_connect(
  [] (net::tcp::Connection_ptr conn) {

    conn->on_read(chunksize, [conn] (auto buf) {
        static size_t count_bytes = 0;

        //printf("chunksize: %zu \n", buf->size());
        assert(buf->size() <= chunksize);
        count_bytes += buf->size();

        if (count_bytes >= num_chunks * chunksize) {

          auto timediff = now() - time_start;
          assert(count_bytes == num_chunks * chunksize);

          double time_sec = timediff.count()/1000.0;
          double mbps = ((count_bytes * 8) / (1024.0 * 1024.0)) / time_sec;

          printf("Server reveived %zu Mb in %f sec. - %f Mbps \n",
                 count_bytes / (1024 * 1024), time_sec,  mbps);
          OS::shutdown();
        }

      });
  });

  printf("*** Linux userspace tcp demo started ***\n");

  time_start = now();
  inet_client.tcp().connect({{"10.0.0.42"}, 80}, [buf](auto conn){
      if (not conn)
        std::abort();

      for (int i = 0; i < num_chunks; i++)
        conn->write(buf);

    });


}
