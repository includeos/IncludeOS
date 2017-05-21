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

///
/// This file tests the base64 module by using the test vectors from:
/// https://tools.ietf.org/pdf/rfc4648.pdf
///

#include <common.cxx>
#include <util/base64.hpp>

CASE("Encode empty string") {
  EXPECT("" == base64::encode(""));
}

CASE("Encode 'f'") {
  EXPECT("Zg==" == base64::encode("f"));
}

CASE("Encode 'fo'") {
  EXPECT("Zm8=" == base64::encode("fo"));
}

CASE("Encode 'foo'") {
  EXPECT("Zm9v" == base64::encode("foo"));
}

CASE("Encode 'foob'") {
  EXPECT("Zm9vYg==" == base64::encode("foob"));
}

CASE("Encode 'fooba'") {
  EXPECT("Zm9vYmE=" == base64::encode("fooba"));
}

CASE("Encode 'foobar'") {
  EXPECT("Zm9vYmFy" == base64::encode("foobar"));
}

CASE("Decode empty string") {
  EXPECT("" == base64::decode<std::string>(""));
}

CASE("Decode 'Zg=='") {
  EXPECT("f" == base64::decode<std::string>("Zg=="));
}

CASE("Decode 'Zm8='") {
  EXPECT("fo" == base64::decode<std::string>("Zm8="));
}

CASE("Decode 'Zm9v'") {
  EXPECT("foo" == base64::decode<std::string>("Zm9v"));
}

CASE("Decode 'Zm9vYg=='") {
  EXPECT("foob" == base64::decode<std::string>("Zm9vYg=="));
}

CASE("Decode 'Zm9vYmE='") {
  EXPECT("fooba" == base64::decode<std::string>("Zm9vYmE="));
}

CASE("Decode 'Zm9vYmFy'") {
  EXPECT("foobar" == base64::decode<std::string>("Zm9vYmFy"));
}

CASE("Decode 'Zm8' (without padding) throws exception") {
  EXPECT_THROWS(base64::decode("Zm8"));
  EXPECT_THROWS_AS(base64::decode("Zm8"),  base64::Decode_error);
}
