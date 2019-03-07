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

#include <net/ws/websocket.hpp>
#include <os.hpp>
#include <util/base64.hpp>
#include <util/sha1.hpp>
#include <cstdint>
#include <net/ws/connector.hpp>

namespace net {

static inline std::string
encode_hash(const std::string& key)
{
  static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  SHA1 sha;
  sha.update(key);
  sha.update(GUID);
  return base64::encode(sha.as_raw());
}

const char* WebSocket::to_string(op_code code)
{
  switch (code) {
    case op_code::CONTINUE:
        return "Continuation frame";
    case op_code::TEXT:
        return "Text frame";
    case op_code::BINARY:
        return "Binary frame";
    case op_code::CLOSE:
        return "Connection close";
    case op_code::PING:
        return "Ping";
    case op_code::PONG:
        return "Pong";
    default:
        return "Reserved (unspecified)";
  }
}

WebSocket_ptr WebSocket::upgrade(http::Request& req, http::Response_writer& writer)
{
  // validate handshake
  auto view = req.header().value("Sec-WebSocket-Version");

  if (view.empty() || view.compare("13") != 0) {
    writer.write_header(http::Bad_Request);
    return nullptr;
  }

  auto key = req.header().value("Sec-WebSocket-Key");
  if (key.empty() || key.size() < 16) {
    writer.write_header(http::Bad_Request);
    return nullptr;
  }

  // create handshake response
  auto& header = writer.header();
  header.set_field(http::header::Connection, "Upgrade");
  header.set_field(http::header::Upgrade,    "WebSocket");
  header.set_field("Sec-WebSocket-Accept", encode_hash(std::string(key)));
  writer.write_header(http::Switching_Protocols);

  auto stream = writer.connection().release();

  // discard streams which can be FIN-WAIT-1
  if (stream->is_connected()) {
    // for now, only accept fully connected streams
    return std::make_unique<WebSocket>(std::move(stream), false);
  }
  return nullptr;
}

WebSocket_ptr WebSocket::upgrade(http::Error err, http::Response& res, http::Connection& conn, const std::string& key)
{
  if (err or res.status_code() != http::Switching_Protocols)
  {
    return nullptr;
  }
  else
  {
    /// validate response
    auto hash = res.header().value("Sec-WebSocket-Accept");
    if (hash.empty() or hash != encode_hash(key))
    {
      return nullptr;
    }
    /// create open websocket
    auto stream = conn.release();
    assert(stream->is_connected());
    // create client websocket and call callback
    return std::make_unique<WebSocket>(std::move(stream), true);
  }
}

std::vector<char> WebSocket::generate_key()
{
  std::vector<char> key(16);
  uint16_t v;
  for (size_t i = 0; i < key.size(); i += sizeof(v))
  {
    v = rand() & 0xffff;
    memcpy(&key[i], &v, sizeof(v));
  }
  return key;
}

http::Server::Request_handler WebSocket::create_request_handler(
  Connect_handler on_connect, Accept_handler on_accept)
{
  return http::Server::Request_handler::make_packed(
    [
      on_connect{std::move(on_connect)},
      on_accept{std::move(on_accept)}
    ]
    (http::Request_ptr req, http::Response_writer_ptr writer)
    {
      if (on_accept)
      {
        const bool accepted = on_accept(writer->connection().peer(),
                                       std::string(req->header().value("Origin")));
        if (not accepted)
        {
          writer->write_header(http::Unauthorized);
          on_connect(nullptr);
          return;
        }
      }
      auto ws = WebSocket::upgrade(*req, *writer);

      on_connect(std::move(ws));
    });
}

http::Basic_client::Response_handler WebSocket::create_response_handler(
  Connect_handler on_connect, std::string key)
{
  return http::Basic_client::Response_handler::make_packed(
    [
      on_connect{std::move(on_connect)},
      key{std::move(key)}
    ]
    (http::Error err, http::Response_ptr res, http::Connection& conn)
    {
      auto ws = WebSocket::upgrade(err, *res, conn, key);

      on_connect(std::move(ws));
    });
}

void WebSocket::connect(
      http::Basic_client&   client,
      uri::URI              remote,
      Connect_handler       callback)
{
  // doesn't have to be extremely random, just random
  std::string key  = base64::encode(generate_key());
  http::Header_set ws_headers {
      {"Host",       std::string(remote)},
      {"Connection", "Upgrade"  },
      {"Upgrade",    "WebSocket"},
      {"Sec-WebSocket-Version", "13"},
      {"Sec-WebSocket-Key",     key }
  };
  // send HTTP request
  client.get(remote, ws_headers,
    WS_client_connector::create_response_handler(std::move(callback), std::move(key)));
}

void WebSocket::read_data(Stream::buffer_t buf)
{
  // silently ignore data for reset connection
  if (this->stream == nullptr) return;

  size_t len = buf->size();
  const uint8_t* data = buf->data();
  while (len)
  {
    if (message != nullptr)
    {
      const size_t written = message->append(data, len);
      len -= written;
      data += len;
    }
    // create new message
    else
    {
      const size_t written = create_message(data, len);

      if(UNLIKELY(message == nullptr))
        return; // Something was invalid, error has been called and stream closed.

      len -= written;
      data += len;
    }

    if (message->is_complete()) {
      finalize_message();
    }
  }
}

size_t WebSocket::Message::append(const uint8_t* data, size_t len)
{
  size_t total = 0;
  // more partial header
  if (UNLIKELY(this->header_complete() == false))
  {
    auto hdr_bytes = std::min(header().header_length() - this->header_length, (int) len);
    memcpy(&header_[this->header_length], data, hdr_bytes);
    this->header_length += hdr_bytes;
    // move forward in buffer
    data += hdr_bytes; len -= hdr_bytes; total += hdr_bytes;
    // if the header became complete, reserve data
    if (this->header_complete()) {
      data_.reserve(header().data_length());
    }
  }
  // fill data with remainder
  if (this->header_complete())
  {
    const size_t insert_size = std::min(data_.capacity() - data_.size(), len);
    data_.insert(data_.end(), data, data + insert_size);
    total += insert_size;
  }
  return total;
}

size_t WebSocket::create_message(const uint8_t* buf, size_t len)
{
  // parse header
  if (len < sizeof(ws_header)) {
    failure("read_data: Header was too short");

    // Consider the remaining buffer as garbage
    return len;
  }

  const auto& hdr = *(const ws_header*) buf;

  if(max_msg_size != 0 and hdr.data_length() > max_msg_size)
  {
    std::string msg{"read: Maximum message size exceeded: "};
    msg.append(std::to_string(max_msg_size)).append(" bytes");

    failure(std::move(msg));
    return 0;
  }

  /*
    printf("Code: %hhu  (%s) (final=%d)\n",
    hdr.opcode(), opcode_string(hdr.opcode()), hdr.is_final());
    printf("Mask: %d  len=%u\n", hdr.is_masked(), hdr.mask_length());
    printf("Payload: len=%u dataofs=%u\n",
    hdr.data_length(), hdr.data_offset());
  */

  // discard invalid messages
  if (hdr.is_masked()) {
    if (clientside == true) {
      failure("Read masked message from server");
      return std::min(hdr.data_length(), len);
    }
  } else if (clientside == false) {
    failure("Read unmasked message from client");
    return std::min(hdr.data_length(), len);
  }

  this->message = std::make_unique<Message>(buf, len);
  return len;
}

void WebSocket::finalize_message()
{
  Expects(message != nullptr and message->is_complete());
  message->unmask();
  const auto& hdr = message->header();
  switch (hdr.opcode()) {
  case op_code::TEXT:
  case op_code::BINARY:
    /// .. call on_read
    if (this->on_read) {
      this->m_busy = true;
      this->on_read(std::move(message));
      this->m_busy = false;
      if (this->m_deferred_close) this->close(this->m_deferred_close);
    }
    return;
  case op_code::CLOSE:
    // there is a message behind the reason, hmm..
    if (hdr.data_length() >= 2) {
      // provide reason to user
      uint16_t reason = htons(*(uint16_t*) message->data());
      this->close(reason);
    }
    else {
      this->close(1000);
    }
    // the websocket is DEAD after close()
    return;
  case op_code::PING:
    if (on_ping(hdr.data(), hdr.data_length())) // if return true, pong back
      write_opcode(op_code::PONG, hdr.data(), hdr.data_length());
    break;
  case op_code::PONG:
    ping_timer.stop();
    if (on_pong != nullptr)
      on_pong(hdr.data(), hdr.data_length());
    break;
  default:
    //printf("Unknown opcode: %d\n", (int) hdr.opcode());
    break;
  }
  message.reset();
}

/** create a websocket message with only the header present
    with the intention of appending the message on the returned buffer */
static Stream::buffer_t create_wsmsg(size_t len, op_code code, bool client)
{
  // generate header length based on buffer length
  const size_t header_len = net::ws_header::header_length(len, client);
  // create shared buffer with position at end of header
  auto buffer = tcp::construct_buffer(header_len);
  // create header on buffer
  auto& hdr = *(new (buffer->data()) ws_header);
  hdr.bits = 0;
  hdr.set_final();
  hdr.set_payload(len);
  hdr.set_opcode(code);
  if (client) {
    hdr.set_masked((os::cycles_since_boot() ^ (uintptr_t) buffer.get()) & 0xffffffff);
  }
  assert(header_len == sizeof(ws_header) + hdr.data_offset());
  return buffer;
}

void WebSocket::write(const char* data, size_t len, op_code code)
{
  if (UNLIKELY(this->stream == nullptr)) {
    failure("write: Already closed");
    return;
  }
  if (UNLIKELY(this->stream->is_writable() == false)) {
    failure("write: Connection not writable");
    return;
  }

  Expects((code == op_code::TEXT or code == op_code::BINARY)
      && "Write currently only supports TEXT or BINARY");

  // fill header
  auto buf = create_wsmsg(len, code, clientside);
  // get data offset & fill in data into buffer
  buf->insert(buf->end(), data, data + len);
  // for client-side we have to mask the data
  if (clientside)
  {
    // mask data to server
    auto& hdr = *(ws_header*) buf->data();
    assert(hdr.is_masked());
    hdr.masking_algorithm(hdr.data());
  }
  /// send everything as shared buffer
  this->stream->write(buf);
}
void WebSocket::write(Stream::buffer_t buffer, op_code code)
{
  if (UNLIKELY(this->stream == nullptr)) {
    failure("write: Already closed");
    return;
  }
  if (UNLIKELY(this->stream->is_writable() == false)) {
    failure("write: Connection not writable");
    return;
  }
  if (UNLIKELY(clientside == true)) {
    failure("write: Client-side does not support sending shared buffers");
    return;
  }

  Expects((code == op_code::TEXT or code == op_code::BINARY)
        && "Write currently only supports TEXT or BINARY");

  /// write header
  auto header = create_wsmsg(buffer->size(), code, false);
  this->stream->write(header);
  /// write shared buffer
  this->stream->write(buffer);
}
bool WebSocket::write_opcode(op_code code, const char* buffer, size_t datalen)
{
  if (UNLIKELY(stream == nullptr || stream->is_writable() == false)) {
    return false;
  }
  /// write header
  auto header = create_wsmsg(datalen, code, clientside);
  this->stream->write(header);
  /// write buffer (if present)
  if (buffer != nullptr && datalen > 0)
      this->stream->write(buffer, datalen);
  return true;
}

WebSocket::WebSocket(net::Stream_ptr stream_ptr, bool client)
  : stream(std::move(stream_ptr)), max_msg_size(0), clientside(client)
{
  assert(stream != nullptr);
  this->stream->on_read(8*1024, {this, &WebSocket::read_data});
  this->stream->on_close({this, &WebSocket::close_callback_once});
}

void WebSocket::close(const uint16_t reason)
{
  if (this->m_busy) {
    this->m_deferred_close = reason; return;
  }
  assert(stream != nullptr);
  /// send CLOSE message
  if (this->stream->is_writable()) {
      uint16_t data = htons(reason);
      this->write_opcode(op_code::CLOSE, (const char*) &data, sizeof(data));
  }
  /// close and unset socket
  this->stream->close();
}
void WebSocket::close_callback_once()
{
  auto close_func = std::move(this->on_close);
  this->reset_callbacks();
  if (close_func) close_func(1000);
}

void WebSocket::reset_callbacks()
{
  this->on_close = nullptr;
  this->on_error = nullptr;
  this->on_read  = nullptr;
  this->on_ping  = nullptr;
  this->on_pong  = nullptr;
  this->on_pong_timeout = nullptr;
}

void WebSocket::failure(const std::string& reason)
{
  if (this->stream != nullptr) this->stream->close();
  if (this->on_error) on_error(reason);
}

const char* WebSocket::status_code(uint16_t code)
{
  switch (code) {
  case 1000:
      return "Closed";
  case 1001:
      return "Going away";
  case 1002:
      return "Protocol error";
  case 1003:
      return "Cannot accept data";
  case 1004:
      return "Reserved";
  case 1005:
      return "Status code not present";
  case 1006:
      return "Connection closed abnormally";
  case 1007:
      return "Non UTF-8 data received";
  case 1008:
      return "Message violated policy";
  case 1009:
      return "Message too big";
  case 1010:
      return "Missing extension";
  case 1011:
      return "Internal server error";
  case 1015:
      return "TLS handshake failure";
  default:
      return "Unknown status code";
  }
}

} // net
