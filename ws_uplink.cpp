// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include "ws_uplink.hpp"
#include "common.hpp"

#include <memdisk>

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#ifndef RAPIDJSON_THROWPARSEEXCEPTION
  #define RAPIDJSON_THROWPARSEEXCEPTION 1
#endif

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <os>
#include <util/sha1.hpp>

namespace uplink {

  const std::string WS_uplink::UPLINK_CFG_FILE{"config.json"};

  void* UPDATE_LOC = (void*) 0x3200000; // at 50mb

  WS_uplink::WS_uplink(net::Inet<net::IP4>& inet)
    : id_{inet.link_addr().to_string()},
      parser_({this, &WS_uplink::handle_transport})
  {
    Expects(inet.ip_addr() != 0 && "Network interface not configured");

    read_config();

    start(inet);

    /*parser_.on_header = [](const auto& hdr) {
      MYINFO("Header: Code: %u Len: %u", static_cast<uint8_t>(hdr.code), hdr.length);
    };*/
  }

  void WS_uplink::start(net::Inet<net::IP4>& inet) {
    MYINFO("Starting WS uplink on %s with ID: %s", 
      inet.ifname().c_str(), id_.c_str());

    Expects(not config_.url.empty());

    if(liu::LiveUpdate::is_resumable(UPDATE_LOC))
    {
      MYINFO("Found resumable state, try restoring...");
      auto success = liu::LiveUpdate::resume(UPDATE_LOC, {this, &WS_uplink::restore});
      CHECK(success, "Success");
    }

    client_ = std::make_unique<http::Client>(inet.tcp(), 
      http::Client::Request_handler{this, &WS_uplink::inject_token});
    
    auth();
  }

  void WS_uplink::store(liu::Storage& store, const liu::buffer_t*)
  {
    liu::Storage::uid id = 0;

    // BINARY HASH
    store.add_string(id++, binary_hash_);
  }

  void WS_uplink::restore(liu::Restore& store)
  {
    // BINARY HASH
    binary_hash_ = store.as_string(); store.go_next();
  }

  std::string WS_uplink::auth_data() const
  {
    return "{ \"id\": \"" + id_ + "\", \"key\": \"" + config_.token + "\"}";
  }

  void WS_uplink::auth()
  {
    std::string url{"http://"};
    url.append(config_.url).append("/auth");

    //static const std::string auth_data{"{ \"id\": \"testor\", \"key\": \"kappa123\"}"};

    MYINFO("Sending auth request to %s", url.c_str());

    client_->post(http::URI{url}, 
      { {"Content-Type", "application/json"} }, 
      auth_data(), 
      {this, &WS_uplink::handle_auth_response});
  }

  void WS_uplink::handle_auth_response(http::Error err, http::Response_ptr res, http::Connection&)
  {
    if(err)
    {
      MYINFO("Auth failed - %s\n", err.to_string().c_str());
      return;
    }

    if(res->status_code() != http::OK)
    {
      MYINFO("Auth failed - %s\n", res->to_string().c_str());
      return;
    } 
    
    MYINFO("Auth success (token received)");
    token_ = res->body().to_string();

    dock();
  }

  void WS_uplink::dock()
  {
    Expects(not token_.empty() and client_ != nullptr);

    std::string url{"ws://"};
    url.append(config_.url).append("/dock");

    MYINFO("Dock attempt to %s", url.c_str());

    net::WebSocket::connect(*client_, http::URI{url}, {this, &WS_uplink::establish_ws});
  }

  void WS_uplink::establish_ws(net::WebSocket_ptr ws)
  {
    if(ws == nullptr) {
      MYINFO("Failed to establish websocket");
      return;
    }

    ws_ = std::move(ws);
    ws_->on_read = {this, &WS_uplink::parse_transport};
    ws_->on_error = [](const auto& reason) {
      printf("WS err: %s\n", reason.c_str());
    };

    MYINFO("Websocket established");

    send_ident();
  }

  void WS_uplink::parse_transport(net::WebSocket::Message_ptr msg)
  {
    parser_.parse(msg->data(), msg->size());
  }

  void WS_uplink::handle_transport(Transport_ptr t)
  {
    if(UNLIKELY(t == nullptr))
    {
      MYINFO("Something went terribly wrong...");
      return;
    }
    
    MYINFO("New transport (%lu bytes)", t->size());
    switch(t->code())
    {
      case Transport_code::UPDATE:
      {
        INFO2("Update received - commencing update...");
        
        update({t->begin(), t->end()});
        return;
      }

      default:
      {
        INFO2("Bad transport\n");
      }
    }
  }

  void WS_uplink::update(const std::vector<char>& buffer)
  {
    static SHA1 checksum;
    checksum.update(buffer);
    binary_hash_ = checksum.as_hex();

    // do the update
    liu::LiveUpdate::begin(UPDATE_LOC, buffer, {this, &WS_uplink::store});
  }

  void WS_uplink::read_config()
  {
    MYINFO("Reading uplink config from %s", UPLINK_CFG_FILE.c_str());
    auto& disk = fs::memdisk();
    
    if (not disk.fs_ready())
    {
      disk.init_fs([] (auto err, auto&) {
        Expects(not err && "Error occured when mounting FS.");
      });  
    }

    auto cfg = disk.fs().stat(UPLINK_CFG_FILE); // name hardcoded for now
    
    Expects(cfg.is_file() && "File not found.");
    Expects(cfg.size() && "File is empty.");

    auto content = cfg.read();

    parse_config(content);
  }

  void WS_uplink::parse_config(const std::string& json)
  {
    using namespace rapidjson;
    Document doc;
    doc.Parse(json.data());

    Expects(doc.IsObject() && "Malformed config");

    Expects(doc.HasMember("uplink") && "Missing member \"uplink\"");

    auto& cfg = doc["uplink"];

    Expects(cfg.HasMember("url") && cfg.HasMember("token") && "Missing url or/and token");

    config_.url   = cfg["url"].GetString();
    config_.token = cfg["token"].GetString();
    
  }

  void WS_uplink::send_ident()
  {
    MYINFO("Sending ident");
    using namespace rapidjson;

    StringBuffer buf;

    Writer<StringBuffer> writer{buf};

    writer.StartObject();

    writer.Key("version");
    writer.String(OS::version());
    
    writer.Key("arch");
    writer.String(OS::arch());
    
    writer.Key("service");
    writer.String(Service::name());
    
    if(not binary_hash_.empty())
    {
      writer.Key("binary");
      writer.String(binary_hash_);  
    }

    writer.EndObject();
    
    std::string str = buf.GetString();

    auto transport = Transport{Header{Transport_code::IDENT, static_cast<uint32_t>(str.size())}};

    transport.load_cargo(str.data(), str.size());

    ws_->write(transport.data().data(), transport.data().size());

  }

}
