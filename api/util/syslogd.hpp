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

#pragma once
#ifndef SYSLOGD_HPP
#define SYSLOGD_HPP

#include <cstdio>
#include <type_traits>
#include <string>

/*
  Need to know about priorities here too (include small header with only the priorities)
  INTERNALLOG also needs to/should be a priority (?)
*/

const int BUFLEN = 2048;
const int TIMELEN = 32;

struct Syslog_facility {
  virtual void syslog(int priority, const std::string& log_message) = 0;
  virtual std::string name() = 0;
  
  Syslog_facility() {}
  Syslog_facility(const char* ident) : ident_{ident} {}
  bool ident_is_set();
  const char* ident() { return ident_; }

private:
  const char* ident_ = nullptr;
};

struct Syslog_user : public Syslog_facility {
  void syslog(int priority, const std::string& log_message) override;
  Syslog_user() : Syslog_facility() {}
  Syslog_user(const char* ident) : Syslog_facility(ident) {}
  std::string name() override;
};

struct Syslog_mail : public Syslog_facility {
  void syslog(int priority, const std::string& log_message) override;
  Syslog_mail() : Syslog_facility() {}
  Syslog_mail(const char* ident) : Syslog_facility(ident) {}
  std::string name() override;
};

struct Syslog {

  // Parameter pack arguments
  template <typename... Args>
  static void syslog(int priority, const char* message, Args&&... args) {
    printf("PARAMETER PACK SYSLOG\n");

    char buf[BUFLEN];
    snprintf(buf, BUFLEN, message, args...);
    syslog(priority, buf);
  };

  // va_list arguments (POSIX)
  static void syslog(int priority, const char* message, va_list args) {
    printf("VA_LIST SYSLOG\n");

    char buf[BUFLEN];
    vsnprintf(buf, BUFLEN, message, args);
    syslog(priority, buf);
  };
  
  static void syslog(int priority, const char* buf);

  template <typename Facility>
  static void openlog(const char* ident, int logopt) {

    // TODO: logopt
    // Do a lot of the same things as with ident (getter +) - depends what to do with this - 
    // it affects the message output (if pid f.ex. is included in the message/log and more)
    // IncludeOS: pid is only 0 or 1 or similar so not that interesting, but still - posix has these
    // options

    static_assert(std::is_base_of<Syslog_facility, Facility>::value, "Facility is not base of Syslog_facility");
    last_open = std::make_unique<Facility>(ident);
  }

  static void closelog() {
    // Back to default values:
    // ident_ = nullptr; (Will then be service-name)
    // log_opt = 0;
    // facility = LOG_USER;
    openlog<Syslog_user>(nullptr, 0);
  }

  // TODO
  static bool valid_priority(int priority) {
    /*
      // Check priority:
      // Check for invalid bits
      if (priority & ~(LOG_PRIMASK|LOG_FACMASK)) {

        // INTERNALLOG IS A PRIORITY, NOT A FACILITY

        syslog(INTERNALLOG, "syslog: unknown facility/priority %x", priority);
        priority &= LOG_PRIMASK|LOG_FACMASK;
      }

      // Check priority against setlogmask values
      // If priority doesn't "match" log_mask, then don't log -
      // hasn't "valid" priority
      if (!(LOG_MASK(LOG_PRI(priority)) & log_mask))
        return;

      //int saved_errno = errno;

      // Set the default facility if none specified
      if ((priority & LOG_FACMASK) == 0)
        priority |= facility_;
    */

    return true;
  }

private:
  static std::unique_ptr<Syslog_facility> last_open;
};

#endif