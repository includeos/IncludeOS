#pragma once
#ifndef UTIL_SYSLOGD_HPP
#define UTIL_SYSLOGD_HPP

#include <cstdio>
#include <string>
#include <regex>
#include <stdarg.h>

#include <syslog.h> // POSIX symbolic constants
#include "syslog_facility.hpp"

class Syslog {
public:
  static void set_facility(std::unique_ptr<Syslog_facility> facility) {
    fac_ = std::move(facility);
  }

  static void settings(const net::Addr& dest_ip, const uint16_t dest_port) {
    fac_->settings(dest_ip, dest_port);
  }

  static const net::Addr& ip() {
    return fac_->ip();
  }

  static uint16_t port() {
    return fac_->port();
  }

  // va_list arguments (POSIX)
  static void syslog(int priority, const char* message, va_list args);

  __attribute__ ((format (printf, 2, 3)))
  static void syslog(int priority, const char* buf, ...);

  static void openlog(const char* ident, int logopt, int facility);

  static void closelog();

  static bool valid_priority(int priority) noexcept {
    return not ((priority < LOG_EMERG) or (priority > LOG_DEBUG));
  }

  static bool valid_logopt(int logopt) noexcept {
    return (logopt & LOG_PID or logopt & LOG_CONS or logopt & LOG_NDELAY or
      logopt & LOG_ODELAY or logopt & LOG_NOWAIT or logopt & LOG_PERROR);
  }

  static bool valid_facility(int facility) noexcept {
    return ( (facility >= LOG_KERN and facility <= LOG_CRON ) or (facility >= LOG_LOCAL0 and facility <= LOG_LOCAL7) );
  }

private:
  static std::unique_ptr<Syslog_facility> fac_;

}; // < Syslog

#endif
