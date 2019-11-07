
#include <util/config.hpp>
#include <rapidjson/error/en.h>
#include <memory>
#include <info>

extern char _CONFIG_JSON_START_;
extern char _CONFIG_JSON_END_;

const Config& Config::get() noexcept
{
  static Config config{&_CONFIG_JSON_START_, &_CONFIG_JSON_END_};
  return config;
}

inline std::unique_ptr<rapidjson::Document> parse_doc()
{
  const auto& cfg = Config::get();

  if(cfg.empty())
    return nullptr;

  auto doc = std::make_unique<rapidjson::Document>();
  doc->Parse(cfg.data());
  if(doc->HasParseError())
  {
    INFO("Config", "Parse error @ offset %zu: %s",
      doc->GetErrorOffset(), rapidjson::GetParseError_En(doc->GetParseError()));
  }
  assert(doc->IsObject() && "Bad formatted config");
  return doc;
}

const rapidjson::Document& Config::doc()
{
  static std::unique_ptr<rapidjson::Document> ptr{parse_doc()};

  assert(ptr != nullptr && "No config found");

  return *ptr;
}
