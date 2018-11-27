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
#include <system_log>
#include <isotime>
#include <net/interfaces>

#include <memdisk>
#include <net/openssl/init.hpp>

static SSL_CTX* init_ssl_context(const std::string& certs_path, bool verify)
{
  MYINFO("Reading certificates from disk @ %s. Verify certs: %s",
    certs_path.c_str(), verify ? "YES" : "NO");

  auto& disk = fs::memdisk();
  disk.init_fs([] (auto err, auto&) {
    Ensures(!err && "Error init filesystem");
  });

  auto ents = disk.fs().ls(certs_path);

  int files = 0;
  for(auto& ent : ents) {
    if(not ent.is_file())
      continue;
    files++;
  }
  INFO2("%d certificates", files);

  Expects(files > 0 && "No files found on disk");

  // initialize client context
  openssl::init();
  return openssl::create_client(ents, verify);
}

namespace uplink {
  constexpr std::chrono::seconds WS_uplink::heartbeat_interval;

  WS_uplink::WS_uplink(Config config)
    : config_{std::move(config)},
      inet_{config_.get_stack()},
      id_{__arch_system_info().uuid},
      parser_({this, &WS_uplink::handle_transport}),
      heartbeat_timer({this, &WS_uplink::on_heartbeat_timer})
  {
#if defined(LIVEUPDATE)
    if(liu::LiveUpdate::is_resumable() && OS::is_live_updated())
    {
      MYINFO("Found resumable state, try restoring...");
      liu::LiveUpdate::resume("uplink", {this, &WS_uplink::restore});

      if(liu::LiveUpdate::partition_exists("conntrack"))
        liu::LiveUpdate::resume("conntrack", {this, &WS_uplink::restore_conntrack});
    }
#endif
    Log::get().set_flush_handler({this, &WS_uplink::send_log});

#if defined(LIVEUPDATE)
    liu::LiveUpdate::register_partition("uplink", {this, &WS_uplink::store});
#endif
    CHECK(config_.reboot, "Reboot on panic");
    if(config_.reboot)
      OS::set_panic_action(OS::Panic_action::reboot);
#if defined(LIVEUPDATE)
    CHECK(config_.serialize_ct, "Serialize Conntrack");
    if(config_.serialize_ct)
      liu::LiveUpdate::register_partition("conntrack", {this, &WS_uplink::store_conntrack});
#endif

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

  void WS_uplink::start(net::Inet& inet) {
    MYINFO("Starting WS uplink on %s with ID %s",
      inet.ifname().c_str(), id_.c_str());

    Expects(inet.ip_addr() != 0 && "Network interface not configured");

    if(config_.url.scheme_is_secure())
    {
      auto* ssl_context = init_ssl_context(config_.certs_path, config_.verify_certs);
      Expects(ssl_context != nullptr && "Secure URL given but no valid certificates found");

      client_ = std::make_unique<http::Client>(inet.tcp(), ssl_context,
        http::Basic_client::Request_handler{this, &WS_uplink::inject_token});
    }
    else
    {
      client_ = std::make_unique<http::Basic_client>(inet.tcp(),
        http::Basic_client::Request_handler{this, &WS_uplink::inject_token});
    }

    auth();
  }
#if defined(LIVEUPDATE)
  void WS_uplink::store(liu::Storage& store, const liu::buffer_t*)
  {
    // BINARY HASH
    store.add_string(0, update_hash_);
    // nanos timestamp of when update begins
    store.add<uint64_t> (1, OS::nanos_since_boot());
    // statman
    auto& stm = Statman::get();
    // increment number of updates performed
    try {
      ++stm.get_by_name("system.updates");
    }
    catch (const std::exception& e)
    {
      ++stm.create(Stat::UINT32, "system.updates");
    }
    // store all stats
    stm.store(2, store);
    // go to end
    store.put_marker(100);
  }

  void WS_uplink::restore(liu::Restore& store)
  {
    // BINARY HASH
    binary_hash_ = store.as_string(); store.go_next();

    // calculate update cycles taken
    uint64_t prev_nanos = store.as_type<uint64_t> (); store.go_next();
    this->update_time_taken = OS::nanos_since_boot() - prev_nanos;
    // statman
    if (!store.is_end())
    {
      Statman::get().restore(store);
    }
    // done marker
    store.pop_marker(100);

    INFO2("Update took %.3f millis", this->update_time_taken / 1.0e6);
  }
#endif
  std::string WS_uplink::auth_data() const
  {
    return "{ \"id\": \"" + id_ + "\", \"key\": \"" + config_.token + "\"}";
  }

  void WS_uplink::auth()
  {
    const static std::string endpoint{"/auth"};

    uri::URI url{config_.url};
    url << endpoint;

    MYINFO("[ %s ] Sending auth request to %s", isotime::now().c_str(), url.to_string().c_str());
    http::Basic_client::Options options;
    options.timeout = 15s;

    client_->post(url,
      { {"Content-Type", "application/json"} },
      auth_data(),
      {this, &WS_uplink::handle_auth_response},
      options);
  }

  void WS_uplink::handle_auth_response(http::Error err, http::Response_ptr res, http::Connection&)
  {
    if(err)
    {
      MYINFO("[ %s ] Auth failed - %s", isotime::now().c_str(), err.to_string().c_str());
      retry_auth();
      return;
    }

    if(res->status_code() != http::OK)
    {
      MYINFO("[ %s ] Auth failed - %s", isotime::now().c_str(), res->to_string().c_str());
      retry_auth();
      return;
    }

    retry_backoff = 0;

    MYINFO("[ %s ] Auth success (token received)", isotime::now().c_str());
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

    const static std::string endpoint{"/dock"};

    // for now, build the websocket url based on the auth url.
    // maybe this will change in the future, and the ws url will have it's own
    // entry in the config
    std::string scheme = (config_.url.scheme_is_secure()) ? "wss://" : "ws://";
    uri::URI url{scheme + config_.url.host_and_port() + endpoint};

    MYINFO("[ %s ] Dock attempt to %s", isotime::now().c_str(), url.to_string().c_str());

    net::WebSocket::connect(*client_, url, {this, &WS_uplink::establish_ws});
  }

  void WS_uplink::establish_ws(net::WebSocket_ptr ws)
  {
    if(ws == nullptr) {
      MYINFO("[ %s ] Failed to establish websocket", isotime::now().c_str());
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

    MYINFO("[ %s ] Websocket established", isotime::now().c_str());

    send_ident();

    send_uplink();

    ws_->on_ping = {this, &WS_uplink::handle_ping};
    ws_->on_pong_timeout = {this, &WS_uplink::handle_pong_timeout};

    heart_retries_left = heartbeat_retries;
    last_ping = RTC::now();
    heartbeat_timer.start(std::chrono::seconds(10));

    if(SystemLog::get_flags() & SystemLog::PANIC)
    {
      MYINFO("[ %s ] Found panic in system log", isotime::now().c_str());
      auto log = SystemLog::copy();
      SystemLog::clear_flags();
      send_message(Transport_code::PANIC, log.data(), log.size());
    }
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
    MYINFO("[ %s ] ! Pong timeout. Retries left %i", isotime::now().c_str(), heart_retries_left);
  }

  void WS_uplink::on_heartbeat_timer()
  {

    if (not is_online()) {
      MYINFO("Can't heartbeat on closed connection.");
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
      MYINFO("[ %s ] Malformed WS message, try to re-establish", isotime::now().c_str());
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
      MYINFO("[ %s ] Something went terribly wrong...", isotime::now().c_str());
      return;
    }

    //MYINFO("New transport (%lu bytes)", t->size());
    switch(t->code())
    {
      case Transport_code::UPDATE:
      {
        MYINFO("[ %s ] Update received - commencing update...", isotime::now().c_str());

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

  void WS_uplink::update(std::vector<char> buffer)
  {
    static SHA1 checksum;
    checksum.update(buffer);
    update_hash_ = checksum.as_hex();

    // send a reponse with the to tell we received the update
    auto trans = Transport{Header{Transport_code::UPDATE, static_cast<uint32_t>(update_hash_.size())}};
    trans.load_cargo(update_hash_.data(), update_hash_.size());
    ws_->write(trans.data().data(), trans.data().size());

    // make sure to flush the driver rings so there is room for the next packets
    inet_.nic().flush();
    // can't wait for defered log flush due to liveupdating
    uplink::Log::get().flush();
    // close the websocket (and tcp) gracefully
    ws_->close();
    // make sure both the log and the close is flushed before updating
    inet_.nic().flush();

#if defined(LIVEUPDATE)
    // do the update
    try {
      liu::LiveUpdate::exec(std::move(buffer));
    }
    catch (const std::exception& e) {
      INFO2("LiveUpdate::exec() failed: %s\n", e.what());
      liu::LiveUpdate::restore_environment();
      // establish new connection
      this->auth();
    }
#endif
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
    MYINFO("[ %s ] Sending ident", isotime::now().c_str());
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

    if(not config_.tag.empty())
    {
      writer.Key("tag");
      writer.String(config_.tag);
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

    auto& stacks = net::Interfaces::get();
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
    MYINFO("[ %s ] Sending uplink", isotime::now().c_str());

    auto str = config_.serialized_string();

    MYINFO("%s", str.c_str());

    auto transport = Transport{Header{Transport_code::UPLINK, static_cast<uint32_t>(str.size())}};
    transport.load_cargo(str.data(), str.size());
    ws_->write(transport.data().data(), transport.data().size());
  }

  void WS_uplink::send_message(Transport_code code, const char* data, size_t len)
  {
    if(UNLIKELY(not is_online()))
      return;

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
    for(auto& stacks : net::Interfaces::get()) {
      for(auto& stack : stacks)
      {
        if(stack.second != nullptr and stack.second->conntrack() != nullptr)
          return stack.second->conntrack();
      }
    }
    return nullptr;
  }
#if defined(LIVEUPDATE)
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
#endif
}
