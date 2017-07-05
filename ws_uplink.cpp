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
#include <kernel/pci_manager.hpp>
#include <hw/pci_device.hpp>
#include <kernel/cpuid.hpp>
#include <statman>

namespace uplink {

  const std::string WS_uplink::UPLINK_CFG_FILE{"config.json"};

  void* UPDATE_LOC = (void*) 0x3200000; // at 50mb

  WS_uplink::WS_uplink(net::Inet<net::IP4>& inet)
    : inet_{inet}, id_{inet.link_addr().to_string()},
      parser_({this, &WS_uplink::handle_transport})
  {
    OS::add_stdout({this, &WS_uplink::send_log});

    read_config();

    if(inet_.is_configured())
    {
      start(inet);
    }
    // if not, register on config event
    else
    {
      MYINFO("Interface not yet configured, starts when ready.");
      inet_.on_config({this, &WS_uplink::start});
    }
  }

  void WS_uplink::start(net::Inet<net::IP4>& inet) {
    MYINFO("Starting WS uplink on %s with ID: %s",
      inet.ifname().c_str(), id_.c_str());

    Expects(inet.ip_addr() != 0 && "Network interface not configured");
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
      MYINFO("Auth failed - %s", err.to_string().c_str());
      retry_auth();
      return;
    }

    if(res->status_code() != http::OK)
    {
      MYINFO("Auth failed - %s", res->to_string().c_str());
      retry_auth();
      return;
    }

    retry_backoff = 0;

    MYINFO("Auth success (token received)");
    token_ = res->body().to_string();

    dock();
  }

  void WS_uplink::retry_auth()
  {
    if(retry_backoff < 6)
      ++retry_backoff;

    std::chrono::seconds secs{5 * retry_backoff};

    MYINFO("Retry auth in %lld seconds...", secs.count());
    retry_timer.restart(secs, {this, &WS_uplink::auth});
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
      retry_auth();
      return;
    }

    ws_ = std::move(ws);
    ws_->on_read = {this, &WS_uplink::parse_transport};
    ws_->on_error = [](const auto& reason) {
      MYINFO("(WS err) %s", reason.c_str());
    };

    ws_->on_close = {this, &WS_uplink::handle_ws_close};

    MYINFO("Websocket established");

    flush_log();

    send_ident();

    send_uplink();
  }

  void WS_uplink::handle_ws_close(uint16_t code)
  {
    (void) code;
    auth();
  }

  void WS_uplink::parse_transport(net::WebSocket::Message_ptr msg)
  {
    if(msg != nullptr) {
      parser_.parse(msg->data(), msg->size());
    }
    else {
      MYINFO("Malformed WS message, try to re-establish");
      send_error("WebSocket error");
      ws_->close();
      ws_ = nullptr;
      dock();
    }
  }

  void WS_uplink::handle_transport(Transport_ptr t)
  {
    if(UNLIKELY(t == nullptr))
    {
      MYINFO("Something went terribly wrong...");
      return;
    }

    //MYINFO("New transport (%lu bytes)", t->size());
    switch(t->code())
    {
      case Transport_code::UPDATE:
      {
        MYINFO("Update received - commencing update...");

        update({t->begin(), t->end()});
        return;
      }

      case Transport_code::STATS:
      {
        send_stats();
        break;
      }

      default:
      {
        INFO2("Bad transport");
      }
    }
  }

  void WS_uplink::update(const std::vector<char>& buffer)
  {
    static SHA1 checksum;
    checksum.update(buffer);
    binary_hash_ = checksum.as_hex();

    // send a reponse with the to tell we received the update
    auto trans = Transport{Header{Transport_code::UPDATE, static_cast<uint32_t>(binary_hash_.size())}};
    trans.load_cargo(binary_hash_.data(), binary_hash_.size());
    ws_->write(trans.data().data(), trans.data().size());

    ws_->close();
    // do the update
    Timers::oneshot(std::chrono::milliseconds(10), [this, buffer] (auto) {
      liu::LiveUpdate::begin(UPDATE_LOC, buffer, {this, &WS_uplink::store});
    });
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

  template <typename Writer, typename Stack_ptr>
  void serialize_stack(Writer& writer, const Stack_ptr& stack)
  {
    if(stack != nullptr)
    {
      writer.StartObject();

      writer.Key("name");
      writer.String(stack->ifname());

      writer.Key("addr");
      writer.String(stack->ip_addr().str());

      writer.Key("netmask");
      writer.String(stack->netmask().str());

      writer.Key("gateway");
      writer.String(stack->gateway().str());

      writer.Key("dns");
      writer.String(stack->dns_addr().str());

      writer.Key("mac");
      writer.String(stack->link_addr().to_string());

      writer.Key("driver");
      writer.String(stack->nic().driver_name());

      writer.EndObject();
    }
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

    writer.Key("service");
    writer.String(Service::name());

    if(not binary_hash_.empty())
    {
      writer.Key("binary");
      writer.String(binary_hash_);
    }

    writer.Key("arch");
    writer.String(OS::arch());

    // CPU Features
    auto features = CPUID::detect_features_str();
    writer.Key("cpu_features");
    writer.StartArray();
    for (auto f : features) {
      writer.String(f);
    }
    writer.EndArray();

    // PCI devices
    auto devices = PCI_manager::devices();
    writer.Key("devices");
    writer.StartArray();
    for (auto* dev : devices) {
      writer.String(dev->to_string());
    }
    writer.EndArray();

    // Network
    writer.Key("net");

    writer.StartArray();

    auto& stacks = net::Super_stack::inet().ip4_stacks();
    for(const auto& stack : stacks) {
      serialize_stack(writer, stack);
    }

    writer.EndArray();



    writer.EndObject();

    std::string str = buf.GetString();


    MYINFO("%s", str.c_str());

    send_message(Transport_code::IDENT, str.data(), str.size());
  }

  void WS_uplink::send_uplink() {
    MYINFO("Sending uplink");
    using namespace rapidjson;

    StringBuffer buf;
    Writer<StringBuffer> writer{buf};

    writer.StartObject();

    writer.Key("url");
    writer.String(config_.url);

    writer.Key("token");
    writer.String(config_.token);

    writer.EndObject();

    std::string str = buf.GetString();

    MYINFO("%s", str.c_str());

    auto transport = Transport{Header{Transport_code::UPLINK, static_cast<uint32_t>(str.size())}};
    transport.load_cargo(str.data(), str.size());
    ws_->write(transport.data().data(), transport.data().size());
  }

  void WS_uplink::send_message(Transport_code code, const char* data, size_t len) {
    auto transport = Transport{Header{code, static_cast<uint32_t>(len)}};

    transport.load_cargo(data, len);

    ws_->write(transport.data().data(), transport.data().size());
  }

  void WS_uplink::send_error(const std::string& err)
  {
    send_message(Transport_code::ERROR, err.c_str(), err.size());
  }

  void WS_uplink::send_log(const char* data, size_t len)
  {
    if(is_online() and ws_->get_connection()->is_writable())
    {
      send_message(Transport_code::LOG, data, len);
    }
    else
    {
      // buffer for later
      logbuf_.insert(logbuf_.end(), data, data+len);
    }
  }

  void WS_uplink::flush_log()
  {
    if(not logbuf_.empty())
    {
      send_message(Transport_code::LOG, logbuf_.data(), logbuf_.size());
      logbuf_.clear();
    }
  }

  void WS_uplink::panic(const char* why){
    MYINFO("WS_uplink sending panic\n");
    send_message(Transport_code::PANIC, why, strlen(why));
    ws_->close();
    inet_.nic().flush();
  }

  void WS_uplink::send_stats()
  {
    using namespace rapidjson;

    StringBuffer buf;
    Writer<StringBuffer> writer{buf};

    writer.StartArray();
    auto& statman = Statman::get();
    for(auto it = statman.begin(); it != statman.end(); ++it)
    {
      auto& stat = *it;
      writer.StartObject();

      writer.Key("name");
      writer.String(stat.name());

      writer.Key("value");
      switch(stat.type()) {
        case Stat::UINT64:  writer.Uint64(stat.get_uint64()); break;
        case Stat::UINT32:  writer.Uint(stat.get_uint32()); break;
        case Stat::FLOAT:   writer.Double(stat.get_float()); break;
      }

      writer.EndObject();
    }
    writer.EndArray();

    std::string str = buf.GetString();

    send_message(Transport_code::STATS, str.data(), str.size());
  }

}
