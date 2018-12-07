#include "balancer.hpp"
#include <stdexcept>
#include <net/inet>
#include <net/tcp/stream.hpp>
#include <net/s2n/stream.hpp>

#define LB_VERBOSE 0
#if LB_VERBOSE
#define LBOUT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define LBOUT(fmt, ...) /** **/
#endif

using namespace liu;

namespace microLB
{
  void Nodes::serialize(Storage& store)
  {
    store.add<int64_t>(100, this->session_total);
    store.put_marker(100);

    LBOUT("Serialize %llu sessions\n", this->session_cnt);
    store.add_int(102, this->session_cnt);

    int alive = 0;
    for(auto& session : sessions)
    {
      if(session.is_alive())
      {
        session.serialize(store);
        ++alive;
      }
    }
    assert(alive == this->session_cnt
      && "Mismatch between number of serialized sessions and actual number serialized");
  }

  void Session::serialize(Storage& store)
  {
    store.add_stream(*incoming);
    store.add_stream(*outgoing);
    store.put_marker(120);
  }

  inline void serialize_stream(liu::Storage& store, net::Stream& stream)
  {
    const int subid = stream.serialization_subid();
    switch (subid) {
      case net::tcp::Stream::SUBID: // TCP
          store.add_stream(stream);
          break;
      case s2n::TLS_stream::SUBID:  // S2N
          store.add_stream(*stream.bottom_transport());
          store.add_stream(stream);
          break;
      default:
          throw std::runtime_error("Unimplemented subid " + std::to_string(subid));
    }
  }

  inline std::unique_ptr<net::Stream>
      deserialize_stream(liu::Restore& store, net::Inet& stack, void* ctx, bool outgoing)
  {
    assert(store.is_stream());
    const int subid = store.get_id();
    std::unique_ptr<net::Stream> result = nullptr;
    
    switch (subid) {
      case net::tcp::Stream::SUBID: // TCP
          result = store.as_tcp_stream(stack.tcp()); store.go_next();
          break;
      case s2n::TLS_stream::SUBID: { // S2N
          auto transp = store.as_tcp_stream(stack.tcp());
          store.go_next();
          result = store.as_tls_stream(ctx, outgoing, std::move(transp));
          store.go_next();
        } break;
      default:
          throw std::runtime_error("Unimplemented subid " + std::to_string(subid));
    }
    return result;
  }

  void Nodes::deserialize(Restore& store, DeserializationHelper& helper)
  {
    /// nodes member fields ///
    this->session_total = store.as_type<int64_t>(); store.go_next();
    store.pop_marker(100);

    /// sessions ///
    const int tot_sessions = store.as_int(); store.go_next();
    // since we are remaking all the sessions, reduce total
    this->session_total -= tot_sessions;

    LBOUT("Deserialize %llu sessions\n", tot_sessions);
    for(auto i = 0; i < static_cast<int>(tot_sessions); i++)
    {
      auto incoming = deserialize_stream(store, *helper.clients, helper.cli_ctx, false);
      auto outgoing = deserialize_stream(store, *helper.nodes,   helper.nod_ctx, true);
      store.pop_marker(120);
      this->create_session(std::move(incoming), std::move(outgoing));
    }
  }

  void Waiting::serialize(liu::Storage& store)
  {
    //store.add_connection(10, this->conn);
    store.add_stream(*this->conn);
    store.add_int(11, (int) readq.size());
    for (auto buffer : readq) {
      store.add_buffer(12, buffer->data(), buffer->size());
    }
    store.put_marker(10);
  }
  Waiting::Waiting(liu::Restore& store, DeserializationHelper& helper)
  {
    this->conn = deserialize_stream(store, *helper.clients, helper.cli_ctx, false);
    int qsize = store.as_int(); store.go_next();
    for (int i = 0; i < qsize; i++)
    {
      auto buf = store.as_buffer(); store.go_next();
      readq.push_back(net::Stream::construct_buffer(buf.begin(), buf.end()));
    }
    store.pop_marker(10);
  }

  void Balancer::serialize(Storage& store, const buffer_t*)
  {
    store.add_int(0, this->throw_counter);
    store.put_marker(0);
    /// wait queue
    store.add_int(1, (int) queue.size());
    for (auto& client : queue) {
      client.serialize(store);
    }
    /// nodes
    nodes.serialize(store);
  }
  void Balancer::deserialize(Restore& store)
  {
    // can't proceed without these two interfaces
    if (de_helper.clients == nullptr || de_helper.nodes == nullptr)
    {
      throw std::runtime_error("Missing deserialization interfaces. Forget to set them?");
    }

    this->throw_counter = store.as_int(); store.go_next();
    store.pop_marker(0);
    /// wait queue
    int wsize = store.as_int(); store.go_next();
    for (int i = 0; i < wsize; i++) {
      queue.emplace_back(store, de_helper);
    }
    /// nodes
    nodes.deserialize(store, this->de_helper);
  }

  void Balancer::resume_callback(liu::Restore& store)
  {
    try {
      this->deserialize(store);
    }
    catch (std::exception& e) {
      fprintf(stderr, "\n!!! Error during microLB resume !!!\n");
      fprintf(stderr, "REASON: %s\n", e.what());
    }
  }

  void Balancer::init_liveupdate()
  {
#ifndef USERSPACE_LINUX
    liu::LiveUpdate::register_partition("microlb", {this, &Balancer::serialize});
    if(liu::LiveUpdate::is_resumable())
    {
      liu::LiveUpdate::resume("microlb", {this, &Balancer::resume_callback});
    }
#endif
  }
}
