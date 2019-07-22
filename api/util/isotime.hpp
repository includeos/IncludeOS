
#pragma once
#ifndef UTIL_ISOTIME_HPP
#define UTIL_ISOTIME_HPP

#include <ctime>
#include <string>
#include <errno.h>

struct isotime
{
  /**
   * @brief      Returns a ISO 8601 UTC datetime string.
   *             Example: 2017-04-10T13:37:00Z
   *
   * @note       Invalid time (too big for the format) will result in a empty string.
   *
   * @param[in]  ts    A timestamp
   *
   * @return     An ISO datetime string, formatted as "YYYY-MM-DDThh:mm:ssZ"
   */
  static std::string to_datetime_string(time_t ts)
  {
    // musl bandaid, bypass strftime setting errno
    const int errno_save = errno;
    char buf[sizeof("2017-04-10T13:37:00Z")];
    const auto res = std::strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&ts));
    errno = errno_save;
    return {buf, res};
  }

  /**
   * @brief      Returns the current time as a datetime according "to_datetime_string"
   *
   * @return     A datetime string representing now
   */
  static std::string now()
  {
    return to_datetime_string(time(0));
  }
};

#endif
