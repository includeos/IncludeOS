// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#include <net/http/websocket.hpp>
#include <kernel/os.hpp>
#include <util/base64.hpp>
#include <util/sha1.hpp>
#include <cstdint>
#include <net/http/ws_connector.hpp>

#define OPCODE_CONTINUE    0
#define OPCODE_TEXT        1
#define OPCODE_BINARY      2
#define OPCODE_CLOSE       8
#define OPCODE_PING        9
#define OPCODE_PONG       10
#define HEADER_MAXLEN     16

const char* opcode_string(uint16_t code)
{
  switch (code) {
  case OPCODE_CONTINUE:
      return "Continuation frame";
  case OPCODE_TEXT:
      return "Text frame";
  case OPCODE_BINARY:
      return "Binary frame";
  case OPCODE_CLOSE:
      return "Connection close";
  case OPCODE_PING:
      return "Ping";
  case OPCODE_PONG:
      return "Pong";
  default:
      return "Reserved (unspecified)";
  }
}

namespace http {

struct ws_header
{
  uint16_t bits;

  bool is_final() const noexcept {
    return (bits >> 7) & 1;
  }
  void set_final() noexcept {
    bits |= 0x80;
    assert(is_final() == true);
  }
  uint16_t payload() const noexcept {
    return (bits >> 8) & 0x7f;
  }
  void set_payload(const size_t len)
  {
    uint16_t pbits = len;
    if      (len > 65535) pbits = 127;
    else if (len > 125)   pbits = 126;
    bits &= 0x80ff;
    bits |= (pbits & 0x7f) << 8;

    if (is_ext())
        *(uint16_t*) vla = __builtin_bswap16(len);
    else if (is_ext2())
        *(uint64_t*) vla = __builtin_bswap64(len);
    assert(data_length() == len);
  }

  bool is_ext() const noexcept {
    return payload() == 126;
  }
  bool is_ext2() const noexcept {
    return payload() == 127;
  }

  bool is_masked() const noexcept {
    return bits & 0x8000;
  }
  void set_masked() noexcept {
    bits |= 0x8000;
    uint32_t mask = OS::cycles_since_boot() & 0xffffffff;
    memcpy(keymask(), &mask, sizeof(mask));
  }

  uint8_t opcode() const noexcept {
    return bits & 0xf;
  }
  void set_opcode(uint8_t code) {
    bits &= ~0xf;
    bits |= code & 0xf;
    assert(opcode() == code);
  }

  bool is_fail() const noexcept {
    return false;
  }

  size_t mask_length() const noexcept {
    return is_masked() ? 4 : 0;
  }
  char* keymask() noexcept {
    return &vla[data_offset() - mask_length()];
  }

  size_t data_offset() const noexcept {
    size_t len = mask_length();
    if (is_ext2()) return len + 8;
    if (is_ext())  return len + 2;
    return len;
  }
  size_t data_length() const noexcept {
    if (is_ext2())
        return __builtin_bswap64(*(uint64_t*) vla) & 0xffffffff;
    if (is_ext())
        return __builtin_bswap16(*(uint16_t*) vla);
    return payload();
  }
  const char* data() const noexcept {
    return &vla[data_offset()];
  }
  char* data() noexcept {
    return &vla[data_offset()];
  }
  void masking_algorithm()
  {
    char* ptr  = data();
    const char* mask = keymask();
    for (size_t i = 0; i < data_length(); i++)
    {
      ptr[i] = ptr[i] xor mask[i & 3];
    }
  }

  size_t reported_length() const noexcept {
    return sizeof(ws_header) + data_offset() + data_length();
  }

  char vla[0];
} __attribute__((packed));

static inline std::string
encode_hash(const std::string& key)
{
  static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  static SHA1  sha;
  sha.update(key + GUID);
  return base64::encode(sha.as_raw());
}


WebSocket_ptr WebSocket::upgrade(Request& req, Response_writer& writer)
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

WebSocket_ptr WebSocket::upgrade(Error err, Response& res, Connection& conn, const std::string& key)
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

static std::string generate16b()
{
  std::string hash; hash.resize(16);
  uint16_t v;
  for (size_t i = 0; i < hash.size(); i += sizeof(v))
  {
    v = rand() & 0xffff;
    memcpy(&hash[i], &v, sizeof(v));
  }
  return hash;
}

std::string WebSocket::generate_key()
{ return generate16b(); }

Server::Request_handler WebSocket::create_request_handler(
  Connect_handler on_connect, Accept_handler on_accept)
{
  auto handler = Server::Request_handler::make_packed(
    [
      on_connect{std::move(on_connect)},
      on_accept{std::move(on_accept)}
    ]
    (Request_ptr req, Response_writer_ptr writer)
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

Client::Response_handler WebSocket::create_response_handler(
  Connect_handler on_connect, std::string key)
{
  auto handler = Client::Response_handler::make_packed(
    [
      on_connect{std::move(on_connect)},
      key{std::move(key)}
    ]
    (Error err, Response_ptr res, Connection& conn)
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
  std::string key  = base64::encode(generate16b());
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
  /// parse header
  if (len < sizeof(ws_header)) {
    failure("read_data: Header was too short");
    return;
  }
  ws_header& hdr = *(ws_header*) buf.get();
  /*
  printf("Code: %hhu  (%s) (final=%d)\n",
          hdr.opcode(), opcode_string(hdr.opcode()), hdr.is_final());
  printf("Mask: %d  len=%u\n", hdr.is_masked(), hdr.mask_length());
  printf("Payload: len=%u dataofs=%u\n",
          hdr.data_length(), hdr.data_offset());
  */
  /// validate payload length
  if (hdr.reported_length() != len) {
    failure("read: Invalid length");
    return;
  }
  /// unmask data (if masked)
  if (hdr.is_masked()) {
      if (clientside == false)
          // apply masking algorithm to demask data
          hdr.masking_algorithm();
      else {
        failure("Read masked message from server");
        return;
      }
  } else if (clientside == false) {
    failure("Read unmasked message from client");
    return;
  }

  switch (hdr.opcode()) {
  case OPCODE_TEXT:
  case OPCODE_BINARY:
      /// .. call on_read
      if (on_read) {
          on_read(hdr.data(), hdr.data_length());
      }
      break;
  case OPCODE_CLOSE:
      // they are angry with us :(
      if (hdr.data_length() >= 2) {
        // provide reason to user
        uint16_t reason = *(uint16_t*) hdr.data();
        if (this->on_close)
            this->on_close(__builtin_bswap16(reason));
      }
      else {
        if (this->on_close) this->on_close(1000);
      }
      // close it down
      this->close();
      return;
  case OPCODE_PING:
      write_opcode(OPCODE_PONG, hdr.data(), hdr.data_length());
      return;
  case OPCODE_PONG:
      return;
  default:
      printf("Unknown opcode: %u\n", hdr.opcode());
      break;
  }
}

static size_t make_header(char* dest, size_t len, uint8_t code, bool client)
{
  new (dest) ws_header;
  auto& hdr = *(ws_header*) dest;
  hdr.bits = 0;
  hdr.set_final();
  hdr.set_payload(len);
  hdr.set_opcode(code);
  if (client) {
    hdr.set_masked();
  }
  // header size + data offset
  return sizeof(ws_header) + hdr.data_offset();
}

void WebSocket::write(const char* buffer, size_t len, mode_t mode)
{
  if (UNLIKELY(this->stream == nullptr)) {
    failure("write: Already closed");
    return;
  }
  if (UNLIKELY(this->stream->is_writable() == false)) {
    failure("write: Connection not writable");
    return;
  }
  uint8_t opcode = (mode == TEXT) ? OPCODE_TEXT : OPCODE_BINARY;
  // allocate header and data at the same time
  auto buf = net::tcp::buffer_t(new uint8_t[HEADER_MAXLEN + len]);
  // fill header
  int  header_len = make_header((char*) buf.get(), len, opcode, clientside);
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
void WebSocket::write(net::tcp::buffer_t buffer, size_t len, mode_t mode)
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
  uint8_t opcode = (mode == TEXT) ? OPCODE_TEXT : OPCODE_BINARY;
  /// write header
  char header[HEADER_MAXLEN];
  int  header_len = make_header(header, len, opcode, false);
  assert(header_len < HEADER_MAXLEN);
  this->stream->write(header, header_len);
  /// write shared buffer
  this->stream->write(buffer, len);
}
bool WebSocket::write_opcode(uint8_t code, const char* buffer, size_t datalen)
{
  if (UNLIKELY(stream == nullptr || stream->is_writable() == false)) {
    return false;
  }
  /// write header
  char header[HEADER_MAXLEN];
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
      this->write_opcode(OPCODE_CLOSE, nullptr, 0);
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

} // http
