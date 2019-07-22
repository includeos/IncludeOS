
#include <util/config.hpp>
#include <cassert>
#include <cstdio>
#include <memory>

static std::unique_ptr<Config> config = nullptr;
static std::unique_ptr<char[]> buffer = nullptr;

const Config& Config::get() noexcept
{
  if (config == nullptr)
  {
    auto* fp = fopen("config.json", "rb");
    assert(fp != nullptr && "Open config.json in source dir");
    fseek(fp, 0L, SEEK_END);
    long int size = ftell(fp);
    rewind(fp);
    // read file into buffer
    buffer.reset(new char[size+1]);
    size_t res = fread(buffer.get(), size, 1, fp);
    assert(res == 1);
    // config needs null-termination
    buffer[size] = 0;
    // create config
    config = std::unique_ptr<Config> (new Config(buffer.get(), buffer.get() + size));
  }
  assert(config != nullptr);
  return *config;
}
