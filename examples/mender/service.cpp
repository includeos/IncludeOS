// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

#include <service>
#include <cstdio>
#include <net/interfaces>
#include <mender/client.hpp>
#include <memdisk>

// Helper function to create a unique identity
std::string mac_identity(MAC::Addr mac)
{
  return "{\"mac\" : \"" + mac.to_string() + "\"}";
}

// Mender server instance (IP:PORT) (local hostnames not supported yet)
net::Socket MENDER_SERVER{{10,0,0,1}, 8090};

// Current running version (artifact_name)
std::string ARTIFACT{"example"};

// The mender client
std::unique_ptr<mender::Client> client;

void Service::start(const std::string&)
{
}

// Client code is done in ::ready() because of timer offset
// when generating private key (compute heavy)
void Service::ready()
{
  auto& inet = net::Interfaces::get(0);
  inet.network_config(
    {  10,  0,  0, 42 },  // IP
    {  255,255,255, 0 },  // Netmask
    {  10,  0,  0,  1 }  // Gateway
  );

  // Mender client currently only supports sync disks (memdisk)
  auto disk = fs::shared_memdisk();
  disk->init_fs([](auto err, auto&) {
    assert(!err && "Could not init FS."); // dont have to panic, can just generate key (but not store..)
  });

  using namespace mender;
  // Create Keystore
  auto ks = std::make_unique<Keystore>(disk, "pk.txt"); // <- load key from "pk.txt"
  //auto ks = std::make_unique<Keystore>(); // <- generates key

  printf("Link: %s\n", inet.link_addr().to_string().c_str());

  // Create the client
  client = std::make_unique<Client>(
    // Auth_manager(Keystore, identity (jsonstr), seqno)
    Auth_manager{std::move(ks), mac_identity(inet.link_addr())},
    // Device(update_loc, current artifact)
    Device{ARTIFACT},
    // TCP instance for HTTP client creation and mender server endpoint
    inet.tcp(), MENDER_SERVER);

  // Save "state" to be restored
  client->on_store([](liu::Storage& store) {
    printf("Adding my state in %p.\n", &store);
  });

  // Restore saved "state"
  client->on_resume([](liu::Restore& store) {
    printf("Resuming my state in %p.\n", &store);
  });

  // Start the client
  client->boot();
}
