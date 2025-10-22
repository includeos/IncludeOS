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
#include <nic_mock.hpp>
#include <net/inet>

using namespace net;

CASE("Default Error indicates no error")
{
  Error err;

  EXPECT(not err);
  EXPECT(not err.is_icmp());
  EXPECT(err.type() == Error::Type::no_error);
  EXPECT(err.what() == "No error");
}

CASE("Creating Error of type Ifdown")
{
  Error err{Error::Type::ifdown, "Ifdown error"};

  EXPECT(err);
  EXPECT(not err.is_icmp());
  EXPECT(err.type() == Error::Type::ifdown);
  EXPECT(err.what() == "Ifdown error");
  EXPECT(err.to_string() == "Ifdown error");
}

CASE("Creating an ICMP_error of type Too Big")
{
  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED, 500};

  EXPECT(err);
  EXPECT(err.is_icmp());
  EXPECT(err.type() == Error::Type::ICMP);
  EXPECT(err.what() == "ICMP error message received");
  EXPECT(err.icmp_type() == icmp4::Type::DEST_UNREACHABLE);
  EXPECT(err.icmp_code() == (uint8_t) icmp4::code::Dest_unreachable::FRAGMENTATION_NEEDED);
  EXPECT(err.icmp_type_str() == "DESTINATION UNREACHABLE (3)");
  EXPECT(err.icmp_code_str() == "FRAGMENTATION NEEDED (4)");
  EXPECT(err.to_string() == "ICMP DESTINATION UNREACHABLE (3): FRAGMENTATION NEEDED (4)");
  EXPECT(err.is_too_big());
  EXPECT(err.pmtu() == 500);
}

CASE("Creating and modifying an ICMP_error of type Port Unreachable")
{
  ICMP_error err{icmp4::Type::DEST_UNREACHABLE, (uint8_t) icmp4::code::Dest_unreachable::PORT};

  EXPECT(err);
  EXPECT(err.is_icmp());
  EXPECT(err.type() == Error::Type::ICMP);
  EXPECT(err.what() == "ICMP error message received");
  EXPECT(err.icmp_type() == icmp4::Type::DEST_UNREACHABLE);
  EXPECT(err.icmp_code() == (uint8_t) icmp4::code::Dest_unreachable::PORT);
  EXPECT(err.icmp_type_str() == "DESTINATION UNREACHABLE (3)");
  EXPECT(err.icmp_code_str() == "PORT (3)");
  EXPECT(err.to_string() == "ICMP DESTINATION UNREACHABLE (3): PORT (3)");
  EXPECT(not err.is_too_big());
  EXPECT(err.pmtu() == 0);

  err.set_icmp_type(icmp4::Type::PARAMETER_PROBLEM);
  err.set_icmp_code((uint8_t) icmp4::code::Parameter_problem::POINTER_INDICATES_ERROR);

  EXPECT(err.icmp_type() == icmp4::Type::PARAMETER_PROBLEM);
  EXPECT(err.icmp_code() == (uint8_t) icmp4::code::Parameter_problem::POINTER_INDICATES_ERROR);
  EXPECT(err.icmp_type_str() == "PARAMETER PROBLEM (12)");
  EXPECT(err.icmp_code_str() == "POINTER INDICATES ERROR (0)");
  EXPECT(err.to_string() == "ICMP PARAMETER PROBLEM (12): POINTER INDICATES ERROR (0)");
  EXPECT(not err.is_too_big());
  EXPECT(err.pmtu() == 0);
}
