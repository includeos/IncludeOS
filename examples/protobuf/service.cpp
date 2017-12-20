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

#include <os>
#include <person.pb.h>

void print_person_info(const char* header, const Person& person)
{
  std::cout << header     << '\n';
  std::cout << "ID: "     << person.id()    << '\n';
  std::cout << "Name: "   << person.name()  << '\n';
  std::cout << "E-mail: " << person.email() << "\n\n";
}

void Service::start()
{
  //---------------------------------------------------------------------------
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  //---------------------------------------------------------------------------
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  std::string data; //< Message buffer

  //---------------------------------------------------------------------------
  // Write message to a std::string
  //---------------------------------------------------------------------------
  Person person;
  person.set_id(1234);
  person.set_name("John Doe");
  person.set_email("jdoe@example.com");
  person.SerializeToString(&data);

  //---------------------------------------------------------------------------
  // Clear message information
  //---------------------------------------------------------------------------
  person.Clear();
  assert(person.id() == 0);
  assert(person.name() == "");
  assert(person.email() == "");
  print_person_info("Cleared message information:", person);

  //---------------------------------------------------------------------------
  // Read message from a std::string
  //---------------------------------------------------------------------------
  person.ParseFromString(data);
  assert(person.id() == 1234);
  assert(person.name() == "John Doe");
  assert(person.email() == "jdoe@example.com");
  print_person_info("Parsed message information:", person);

  //---------------------------------------------------------------------------
  // Print raw message
  //---------------------------------------------------------------------------
  std::cout << "Raw message:\n";
  std::cout << data << '\n';

  //---------------------------------------------------------------------------
  // Optional:  Delete all global objects allocated by libprotobuf.
  //---------------------------------------------------------------------------
  google::protobuf::ShutdownProtobufLibrary();
}
