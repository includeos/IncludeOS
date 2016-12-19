// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

// Syslog_facility weak default printf

#include <util/syslog_facility.hpp>

// Weak
void Syslog_facility::syslog(const std::string& log_message) {
	if (logopt_ & LOG_CONS /*and priority_ == LOG_ERR*/)
		std::cout << log_message.c_str() << '\n';

  if (logopt_ & LOG_PERROR)
    std::cerr << log_message.c_str() << '\n';

	printf("%s\n", log_message.c_str());
}

// Weak
Syslog_facility::~Syslog_facility() {}

// Weak
void Syslog_facility::open_socket() {}

// Weak
void Syslog_facility::close_socket() { sock_ = nullptr; }

// Weak
void Syslog_facility::send_udp_data(const std::string&) {}
