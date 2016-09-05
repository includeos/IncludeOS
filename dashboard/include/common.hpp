#pragma once
#ifndef DASHBOARD_COMMON_HPP
#define DASHBOARD_COMMON_HPP

#include <delegate>
#include <json.hpp> // rapidjson
#include <server/request.hpp>
#include <server/response.hpp>

namespace dashboard {
  using WriteBuffer = rapidjson::StringBuffer;
  using Writer = rapidjson::Writer<WriteBuffer>;
  using Serialize = delegate<void(Writer&)>;
  using RouteCallback = delegate<void(server::Request_ptr, server::Response_ptr)>;
}

#endif
