#include <common.cxx>
#include <util/config.hpp>

char _CONFIG_JSON_START_;
char _CONFIG_JSON_END_;

CASE("Test empty config")
{
  auto& config = Config::get();
  EXPECT(config.data() == &_CONFIG_JSON_START_);
  EXPECT(config.size() != 0);
  EXPECT(config.empty() == false);
}
