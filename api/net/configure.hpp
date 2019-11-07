
#pragma once
#ifndef NET_AUTOCONF_HPP
#define NET_AUTOCONF_HPP

#ifndef RAPIDJSON_HAS_STDSTRING
  #define RAPIDJSON_HAS_STDSTRING 1
#endif

#ifndef RAPIDJSON_THROWPARSEEXCEPTION
  #define RAPIDJSON_THROWPARSEEXCEPTION 1
#endif

#include <rapidjson/document.h>

namespace net {

  /**
   * @brief      Configure interfaces according the json array "net".
   *
   * @details    Example for configuring interface 0 and 1:
   *
   *             "net": [
   *                ["10.0.0.42", "255.255.255.0", "10.0.0.1"],
   *                 "dhcp"
   *             ]
   *
   * @param[in]  net   A rapidjson value of the member "net"
   */
  void configure(const rapidjson::Value& net);

}

#endif
