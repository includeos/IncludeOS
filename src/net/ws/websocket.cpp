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
#include <kernel/os.hpp>
#include <util/base64.hpp>
#include <util/sha1.hpp>
#include <cstdint>
#include <net/ws/connector.hpp>

namespace net {

static inline std::string
encode_hash(const std::string& key)
{
  static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  static SHA1  sha;
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
  if (view == nullptr || view != "13") {
    writer.write_header(http::Bad_Request);
    return nullptr;
  }

  auto key = req.header().value("Sec-WebSocket-Key");
  if (key == nullptr || key.size() < 16) {
    writer.write_header(http::Bad_Request);
    return nullptr;
  }

  // create handshake response
  auto& header = writer.header();
  header.set_field(http::header::Connection, "Upgrade");
  header.set_field(http::header::Upgrade,    "WebSocket");
  header.set_field("Sec-WebSocket-Accept", encode_hash(key.to_string()));
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
  auto handler = http::Server::Request_handler::make_packed(
    [
      on_connect{std::move(on_connect)},
      on_accept{std::move(on_accept)}
    ]
    (http::Request_ptr req, http::Response_writer_ptr writer)
    {
      if (on_accept)
      {
        const bool accepted = on_accept(writer->connection().peer(),
                                       req->header().value("Origin").to_string());
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

  return handler;
}

http::Client::Response_handler WebSocket::create_response_handler(
  Connect_handler on_connect, std::string key)
{
  auto handler = http::Client::Response_handler::make_packed(
    [
      on_connect{std::move(on_connect)},
      key{std::move(key)}
    ]
    (http::Error err, http::Response_ptr res, http::Connection& conn)
    {
      auto ws = WebSocket::upgrade(err, *res, conn, key);

      on_connect(std::move(ws));
    });

  return handler;
}

void WebSocket::connect(
      http::Client&   client,
      uri::URI        remote,
      Connect_handler callback)
{
  // doesn't have to be extremely random, just random
  std::string key  = base64::encode(generate_key());
  http::Header_set ws_headers {
      {"Host",       remote.to_string()},
      {"Connection", "Upgrade"  },
      {"Upgrade",    "WebSocket"},
      {"Sec-WebSocket-Version", "13"},
      {"Sec-WebSocket-Key",     key }
  };
  // send HTTP request
  client.get(remote, ws_headers,
    WS_client_connector::create_response_handler(std::move(callback), std::move(key)));
}

void WebSocket::read_data(net::tcp::buffer_t buf, size_t len)
{
  // silently ignore data from reset connection
  if (this->stream == nullptr) return;

  // parse message
  if (message != nullptr)
  {
    try {
      message->add(reinterpret_cast<char*>(buf.get()), len);
    }
    catch(const WS_error& err)
    {
      failure(err.what());
      on_read(nullptr);
      return;
    }
  }
  // create new message
  else
  {
    // parse header
    if (len < sizeof(ws_header)) {
      failure("read_data: Header was too short");
      return;
    }

    ws_header& hdr = *reinterpret_cast<ws_header*>(buf.get());

    // TODO: Add configuration for this, hardcoded max msgs of 5MB for now
    if (hdr.data_length() > (1024 * 1024 * 5)) {
      failure("read: Maximum message size exceeded (5MB)");
      return;
    }
    /*
    printf("Code: %hhu  (%s) (final=%d)\n",
            hdr.opcode(), opcode_string(hdr.opcode()), hdr.is_final());
    printf("Mask: %d  len=%u\n", hdr.is_masked(), hdr.mask_length());
    printf("Payload: len=%u dataofs=%u\n",
            hdr.data_length(), hdr.data_offset());
    */

    /// unmask data (if masked)
    if (hdr.is_masked()) {
      if (clientside == true) {
        failure("Read masked message from server");
        return;
      }
    } else if (clientside == false) {
      failure("Read unmasked message from client");
      return;
    }

    try
    {
      message = std::make_unique<Message>(reinterpret_cast<char*>(buf.get()), len);
    }
    catch(const WS_error& err)
    {
      failure(err.what());
      on_read(nullptr);
      return;
    }
  }

  if(message->is_complete())
  {
    message->unmask();
    const auto& hdr = message->header();
    switch (hdr.opcode()) {
      case op_code::TEXT:
      case op_code::BINARY:
        /// .. call on_read
        if (on_read) {
            on_read(std::move(message));
        }
        break;
      case op_code::CLOSE:
        // they are angry with us :(
        if (hdr.data_length() >= 2) {
          // provide reason to user
          uint16_t reason = *(uint16_t*) message->data();
          if (this->on_close)
              this->on_close(__builtin_bswap16(reason));
        }
        else {
          if (this->on_close) this->on_close(1000);
        }
        // close it down
        this->close();
        break;
      case op_code::PING:
        write_opcode(op_code::PONG, hdr.data(), hdr.data_length());
        break;
      case op_code::PONG:
        break;
      default:
        printf("Unknown opcode: %u\n", static_cast<unsigned>(hdr.opcode()));
        break;
      }
    message.reset();
  }

}

static size_t make_header(char* dest, size_t len, op_code code, bool client)
{
  new (dest) ws_header;
  auto& hdr = *(ws_header*) dest;
  hdr.bits = 0;
  hdr.set_final();
  hdr.set_payload(len);
  hdr.set_opcode(code);
  if (client) {
    hdr.set_masked(OS::cycles_since_boot() & 0xffffffff);
  }
  // header size + data offset
  return sizeof(ws_header) + hdr.data_offset();
}

void WebSocket::write(const char* buffer, size_t len, op_code code)
{
  if (UNLIKELY(this->stream == nullptr)) {
    failure("write: Already closed");
    return;
  }
  if (UNLIKELY(this->stream->is_writable() == false)) {
    failure("write: Connection not writable");
    return;
  }

  Expects(code == op_code::TEXT or code == op_code::BINARY
    && "Write currently only supports TEXT or BINARY");

  // allocate header and data at the same time
  auto buf = net::tcp::buffer_t(new uint8_t[WS_HEADER_MAXLEN + len]);
  // fill header
  int  header_len = make_header((char*) buf.get(), len, code, clientside);
  // get data offset & fill in data into buffer
  char* data_ptr = (char*) buf.get() + header_len;
  memcpy(data_ptr, buffer, len);
  // for client-side we have to mask the data
  if (clientside)
  {
    // mask data to server
    auto& hdr = *(ws_header*) buf.get();
    assert(hdr.is_masked());
    hdr.masking_algorithm();
  }
  /// send everything as shared buffer
  this->stream->write(buf, header_len + len);
}
void WebSocket::write(net::tcp::buffer_t buffer, size_t len, op_code code)
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

  Expects(code == op_code::TEXT or code == op_code::BINARY
    && "Write currently only supports TEXT or BINARY");

  /// write header
  char header[WS_HEADER_MAXLEN];
  int  header_len = make_header(header, len, code, false);
  assert(header_len < WS_HEADER_MAXLEN);
  this->stream->write(header, header_len);
  /// write shared buffer
  this->stream->write(buffer, len);
}
bool WebSocket::write_opcode(op_code code, const char* buffer, size_t datalen)
{
  if (UNLIKELY(stream == nullptr || stream->is_writable() == false)) {
    return false;
  }
  /// write header
  char header[WS_HEADER_MAXLEN];
  int  header_len = make_header(header, datalen, code, clientside);
  this->stream->write(header, header_len);
  /// write buffer (if present)
  if (buffer != nullptr && datalen > 0)
      this->stream->write(buffer, datalen);
  return true;
}
void WebSocket::tcp_closed()
{
  if (this->on_close != nullptr) this->on_close(1000);
  this->reset();
}

WebSocket::WebSocket(net::Stream_ptr stream_ptr, bool client)
  : stream(std::move(stream_ptr)), clientside(client)
{
  assert(stream != nullptr);
  this->stream->on_read(16384, {this, &WebSocket::read_data});
  this->stream->on_close({this, &WebSocket::tcp_closed});
}

WebSocket::WebSocket(WebSocket&& other)
{
  other.on_close = std::move(on_close);
  other.on_error = std::move(on_error);
  other.on_read  = std::move(on_read);
  other.stream   = std::move(stream);
  other.clientside = clientside;
}
WebSocket::~WebSocket()
{
  if (stream != nullptr && stream->is_connected())
      this->close();
}

void WebSocket::close()
{
  /// send CLOSE message
  if (this->stream->is_writable())
      this->write_opcode(op_code::CLOSE, nullptr, 0);
  /// close and unset socket
  this->stream->close();
  this->reset();
}

void WebSocket::reset()
{
  this->on_close = nullptr;
  this->on_error = nullptr;
  this->on_read  = nullptr;
  stream->reset_callbacks();
  stream->close();
  stream = nullptr;
}

void WebSocket::failure(const std::string& reason)
{
  if (stream != nullptr) stream->close();
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
