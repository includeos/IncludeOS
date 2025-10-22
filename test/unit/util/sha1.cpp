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

#include <common.cxx>
#include <util/sha1.hpp>
#include <util/base64.hpp>

CASE("Rolling checksum verification") {
  SHA1 checksum;
  checksum.update("abc");
  EXPECT(checksum.as_hex() == "a9993e364706816aba3e25717850c26c9cd0d89d");

  checksum.update("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
  EXPECT(checksum.as_hex() == "84983e441c3bd26ebaae4aa1f95129e5e54670f1");

  for (int i = 0; i < 1000000/200; ++i)
  {
      checksum.update("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                     );
  }
  EXPECT(checksum.as_hex() == "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
}

CASE("Vector magic") {
  SHA1 checksum;
  checksum.update("abc");
  auto raw = checksum.as_raw();
  EXPECT(raw.size() == 20);
  EXPECT(memcmp(raw.data(),
  "\xa9\x99\x3e\x36\x47\x06\x81\x6a\xba\x3e\x25\x71\x78\x50\xc2\x6c\x9c\xd0\xd8\x9d",
          raw.size()) == 0);
}

inline std::string encode_hash(const std::string& key)
{
  static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  static SHA1  hash;
  hash.update(key);
  hash.update(GUID);
  return base64::encode(hash.as_raw());
}

CASE("WebSocket Handshake") {
  EXPECT(encode_hash("dGhlIHNhbXBsZSBub25jZQ==")
                 == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}
