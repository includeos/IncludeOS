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
#include <config>
#include "log.hpp"

namespace uplink {
  constexpr std::chrono::seconds WS_uplink::heartbeat_interval;

  WS_uplink::WS_uplink(Config config)
    : config_{std::move(config)},
      inet_{*config_.inet},
      id_{inet_.link_addr().to_string()},
      parser_({this, &WS_uplink::handle_transport}),
      heartbeat_timer({this, &WS_uplink::on_heartbeat_timer})
  {
    if(liu::LiveUpdate::is_resumable() && OS::is_live_updated())
    {
      MYINFO("Found resumable state, try restoring...");
      liu::LiveUpdate::resume("uplink", {this, &WS_uplink::restore});

      if(liu::LiveUpdate::partition_exists("conntrack"))
        liu::LiveUpdate::resume("conntrack", {this, &WS_uplink::restore_conntrack});
    }

    Log::get().set_flush_handler({this, &WS_uplink::send_log});

    liu::LiveUpdate::register_partition("uplink", {this, &WS_uplink::store});

    CHECK(config_.reboot, "Reboot on panic");

    CHECK(config_.serialize_ct, "Serialize Conntrack");
    if(config_.serialize_ct)
      liu::LiveUpdate::register_partition("conntrack", {this, &WS_uplink::store_conntrack});

    if(inet_.is_configured())
    {
      start(inet_);
    }
    // if not, register on config event
    else
    {
      MYINFO("Interface %s not yet configured, starts when ready.", inet_.ifname().c_str());
      inet_.on_config({this, &WS_uplink::start});
    }
  }

  void WS_uplink::start(net::Inet<net::IP4>& inet) {
    MYINFO("Starting WS uplink on %s with ID %s",
      inet.ifname().c_str(), id_.c_str());

    Expects(inet.ip_addr() != 0 && "Network interface not configured");
    Expects(not config_.url.empty());

    client_ = std::make_unique<http::Client>(inet.tcp(),
      http::Client::Request_handler{this, &WS_uplink::inject_token});

    auth();
  }

  void WS_uplink::store(liu::Storage& store, const liu::buffer_t*)
  {
    // BINARY HASH
    store.add_string(0, update_hash_);
    // nanos timestamp of when update begins
    store.add<uint64_t> (1, OS::nanos_since_boot());
  }

  void WS_uplink::restore(liu::Restore& store)
  {
    // BINARY HASH
    binary_hash_ = store.as_string(); store.go_next();

    // calculate update cycles taken
    uint64_t prev_nanos = store.as_type<uint64_t> (); store.go_next();
    this->update_time_taken = OS::nanos_since_boot() - prev_nanos;

    INFO2("Update took %.3f millis", this->update_time_taken / 1.0e6);
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
    token_ = std::string(res->body());

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

    flush_log();

    MYINFO("Websocket established");

    send_ident();

    send_uplink();

    ws_->on_ping = {this, &WS_uplink::handle_ping};
    ws_->on_pong_timeout = {this, &WS_uplink::handle_pong_timeout};

    heart_retries_left = heartbeat_retries;
    last_ping = RTC::now();
    heartbeat_timer.start(std::chrono::seconds(10));
  }

  void WS_uplink::handle_ws_close(uint16_t code)
  {
    (void) code;
    auth();
  }

  bool WS_uplink::handle_ping(const char*, size_t)
  {
    last_ping = RTC::now();
    return true;
  }

  void WS_uplink::handle_pong_timeout(net::WebSocket&)
  {
    heart_retries_left--;
    MYINFO("! Pong timeout. Retries left %i", heart_retries_left);
  }

  void WS_uplink::on_heartbeat_timer()
  {

    if (not is_online()) {
      MYINFO("Can't heartbeat on closed conection. ");
      return;
    }

    if(missing_heartbeat())
    {
      if (not heart_retries_left)
      {
        MYINFO("No reply after %i pings. Reauth.", heartbeat_retries);
        ws_->close();
        auth();
        return;
      }

      auto ping_ok = ws_->ping(std::chrono::seconds(5));

      if (not ping_ok)
      {
        MYINFO("Heartbeat pinging failed. Reauth.");
        auth();
        return;
      }
    }

    heartbeat_timer.start(std::chrono::seconds(10));
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
    update_hash_ = checksum.as_hex();

    // send a reponse with the to tell we received the update
    auto trans = Transport{Header{Transport_code::UPDATE, static_cast<uint32_t>(update_hash_.size())}};
    trans.load_cargo(update_hash_.data(), update_hash_.size());
    ws_->write(trans.data().data(), trans.data().size());
    ws_->close();

    // do the update
    Timers::oneshot(std::chrono::milliseconds(10),
    [this, copy = buffer] (int) {
      try {
        liu::LiveUpdate::exec(copy);
      }
      catch (std::exception& e) {
        INFO2("LiveUpdate::exec() failed: %s\n", e.what());
        liu::LiveUpdate::restore_environment();
        // establish new connection
        this->auth();
      }
    });
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

    const auto& sysinfo = __arch_system_info();
    writer.Key("uuid");
    writer.String(sysinfo.uuid);

    writer.Key("version");
    writer.String(OS::version());

    writer.Key("service");
    writer.String(Service::name());

    if(not binary_hash_.empty())
    {
      writer.Key("binary");
      writer.String(binary_hash_);
    }

    if(update_time_taken > 0)
    {
      writer.Key("update_time_taken");
      writer.Uint64(update_time_taken);
    }

    writer.Key("arch");
    writer.String(OS::arch());

    writer.Key("physical_ram");
    writer.Uint64(sysinfo.physical_memory);

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
      for(const auto& pair : stack)
        serialize_stack(writer, pair.second);
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

    writer.Key("reboot");
    writer.Bool(config_.reboot);

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
    if(not config_.ws_logging)
      return;

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
      if(config_.ws_logging)
      {
        send_message(Transport_code::LOG, logbuf_.data(), logbuf_.size());
      }
      logbuf_.clear();
      logbuf_.shrink_to_fit();
    }
  }

  void WS_uplink::panic(const char* why){
    MYINFO("WS_uplink sending panic\n");
    Log::get().flush();
    send_message(Transport_code::PANIC, why, strlen(why));
    ws_->close();
    inet_.nic().flush();

    if(config_.reboot) OS::reboot();
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

  std::shared_ptr<net::Conntrack> get_first_conntrack()
  {
    for(auto& stacks : net::Super_stack::inet().ip4_stacks()) {
      for(auto& stack : stacks)
      {
        if(stack.second != nullptr and stack.second->conntrack() != nullptr)
          return stack.second->conntrack();
      }
    }
    return nullptr;
  }

  void WS_uplink::store_conntrack(liu::Storage& store, const liu::buffer_t*)
  {
    // NOTE: Only support serializing one conntrack atm
    auto ct = get_first_conntrack();
    if(not ct)
      return;

    liu::buffer_t buf;
    ct->serialize_to(buf);
    store.add_buffer(0, buf);
  }

  void WS_uplink::restore_conntrack(liu::Restore& store)
  {
    // NOTE: Only support deserializing one conntrack atm
    auto ct = get_first_conntrack();
    if(not ct)
      return;

    auto buf = store.as_buffer();
    ct->deserialize_from(buf.data());
  }

}
