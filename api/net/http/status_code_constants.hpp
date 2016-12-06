// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License"),
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

#ifndef HTTP_STATUS_CODE_CONSTANTS_HPP
#define HTTP_STATUS_CODE_CONSTANTS_HPP

namespace http {

enum status_t {
//< 1xx: Informational - Request received, continuing process
Continue            = 100,
Switching_Protocols = 101,
Processing          = 102,

//< 2xx: Success - The action was successfully received, understood, and accepted
OK                  = 200,
Created             = 201,
Accepted            = 202,
Non_Authoritative   = 203,
No_Content          = 204,
Reset_Content       = 205,
Partial_Content     = 206,
Multi_Status        = 207,
Already_Reported    = 208,
IM_Used             = 226,

//< 3xx: Redirection - Further action must be taken in order to complete the request
Multiple_Choices    = 300,
Moved_Permanently   = 301,
Found               = 302,
See_Other           = 303,
Not_Modified        = 304,
Use_Proxy           = 305,
Temporary_Redirect  = 307,
Permanent_Redirect  = 308,

//< 4xx: Client Error - The request contains bad syntax or cannot be fulfilled
Bad_Request                     = 400,
Unauthorized                    = 401,
Payment_Required                = 402,
Forbidden                       = 403,
Not_Found                       = 404,
Method_Not_Allowed              = 405,
Not_Acceptable                  = 406,
Proxy_Authentication_Required   = 407,
Request_Timeout                 = 408,
Conflict                        = 409,
Gone                            = 410,
Length_Required                 = 411,
Precondition_Failed             = 412,
Payload_Too_Large               = 413,
URI_Too_Long                    = 414,
Unsupported_Media_Type          = 415,
Range_Not_Satisfiable           = 416,
Expectation_Failed              = 417,
Misdirected_Request             = 421,
Unprocessable_Entity            = 422,
Locked                          = 423,
Failed_Dependency               = 424,
Upgrade_Required                = 426,
Precondition_Required           = 428,
Too_Many_Requests               = 429,
Request_Header_Fields_Too_Large = 431,

//< 5xx: Server Error - The server failed to fulfill an apparently valid request
Internal_Server_Error           = 500,
Not_Implemented                 = 501,
Bad_Gateway                     = 502,
Service_Unavailable             = 503,
Gateway_Timeout                 = 504,
HTTP_Version_Not_Supported      = 505,
Variant_Also_Negotiates         = 506,
Insufficient_Storage            = 507,
Loop_Detected                   = 508,
Not_Extended                    = 510,
Network_Authentication_Required = 511,
}; //< enum status_t

} //< namespace http

#endif //< HTTP_STATUS_CODE_CONSTANTS_HPP
