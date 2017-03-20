// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#ifndef HTTP_HEADER_FIELDS_HPP
#define HTTP_HEADER_FIELDS_HPP

namespace http {
namespace header {
//------------------------------------------------
using Field = const char*;
//------------------------------------------------
// Request Fields
//------------------------------------------------
extern Field Accept;
extern Field Accept_Charset;
extern Field Accept_Encoding;
extern Field Accept_Language;
extern Field Authorization;
extern Field Cookie;
extern Field Connection;
extern Field Expect;
extern Field From;
extern Field Host;
extern Field HTTP2_Settings;
extern Field If_Match;
extern Field If_Modified_Since;
extern Field If_None_Match;
extern Field If_Range;
extern Field If_Unmodified_Since;
extern Field Max_Forwards;
extern Field Origin;
extern Field Proxy_Authorization;
extern Field Range;
extern Field Referer;
extern Field TE;
extern Field Upgrade;
extern Field User_Agent;
//------------------------------------------------
// Response Fields
//------------------------------------------------
extern Field Accept_Ranges;
extern Field Age;
extern Field Date;
extern Field ETag;
extern Field Location;
extern Field Proxy_Authenticate;
extern Field Retry_After;
extern Field Server;
extern Field Set_Cookie;
extern Field Vary;
extern Field WWW_Authenticate;
//------------------------------------------------
// Entity Fields
//------------------------------------------------
extern Field Allow;
extern Field Content_Encoding;
extern Field Content_Language;
extern Field Content_Length;
extern Field Content_Location;
extern Field Content_MD5;
extern Field Content_Range;
extern Field Content_Type;
extern Field Expires;
extern Field Last_Modified;
//------------------------------------------------
//------------------------------------------------
} //< namespace header
} //< namespace http

#endif //< HTTP_HEADER_FIELDS_HPP
