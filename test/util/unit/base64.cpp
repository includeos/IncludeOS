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

using namespace std;

CASE("Encode empty string") {
  EXPECT("" == base64::encode(""s));
}

CASE("Encode 'f'") {
  EXPECT("Zg==" == base64::encode("f"s));
}

CASE("Encode 'fo'") {
  EXPECT("Zm8=" == base64::encode("fo"s));
}

CASE("Encode 'foo'") {
  EXPECT("Zm9v" == base64::encode("foo"s));
}

CASE("Encode 'foob'") {
  EXPECT("Zm9vYg==" == base64::encode("foob"s));
}

CASE("Encode 'fooba'") {
  EXPECT("Zm9vYmE=" == base64::encode("fooba"s));
}

CASE("Encode 'foobar'") {
  EXPECT("Zm9vYmFy" == base64::encode("foobar"s));
}

CASE("Decode empty string") {
  EXPECT("" == base64::decode<std::string>(""s));
}

CASE("Decode 'Zg=='") {
  EXPECT("f" == base64::decode<std::string>("Zg=="s));
}

CASE("Decode 'Zm8='") {
  EXPECT("fo" == base64::decode<std::string>("Zm8="s));
}

CASE("Decode 'Zm9v'") {
  EXPECT("foo" == base64::decode<std::string>("Zm9v"s));
}

CASE("Decode 'Zm9vYg=='") {
  EXPECT("foob" == base64::decode<std::string>("Zm9vYg=="s));
}

CASE("Decode 'Zm9vYmE='") {
  EXPECT("fooba" == base64::decode<std::string>("Zm9vYmE="s));
}

CASE("Decode 'Zm9vYmFy'") {
  EXPECT("foobar" == base64::decode<std::string>("Zm9vYmFy"s));
}
