#include "ircsplit.hpp"

std::vector<std::string>
ircsplit(const std::string& text, std::string& source)
{
  std::vector<std::string> retv;
  size_t x = 0;
  size_t p = 0;
  // ignore empty messages
  if (text.empty()) return retv;
  // check for source
  if (text[0] == ':')
  {
    // if the input is ":" or too short
    if (text.size() < 3) return retv;
    x = text.find(' ', 1);
    // early return for source-only msg
    if (x == std::string::npos) return retv;
    source = text.substr(x);
    p = x+1;
  }
  // parse remainder
  do
  {
    x = text.find(' ', p+1);
    size_t y = text.find(':', x+1); // find last param

    if (x != std::string::npos && y == x+1)
    {
      // single argument
      retv.emplace_back(&text[p], x-p);
      // ending text argument
      retv.emplace_back(&text[y+1], text.size() - (y+1));
      break;
    }
    else if (x != std::string::npos)
    {
      // single argument
      retv.emplace_back(&text[p], x-p);
    }
    else {
      // last argument
      retv.emplace_back(&text[p], text.size()-p);
    }
    p = x+1;

  } while (x != std::string::npos);

  return retv;
}

std::vector<std::string>
ircsplit(const std::string& text)
{
  std::vector<std::string> retv;
  size_t x = 0;
  size_t p = 0;
  // ignore empty messages
  if (text.empty()) return retv;
  // parse remainder
  do
  {
    x = text.find(' ', p+1);
    size_t y = text.find(':', x+1); // find last param

    if (x != std::string::npos && y == x+1)
    {
      // single argument
      retv.emplace_back(&text[p], x-p);
      // ending text argument
      retv.emplace_back(&text[y+1], text.size() - (y+1));
      break;
    }
    else if (x != std::string::npos)
    {
      // single argument
      retv.emplace_back(&text[p], x-p);
    }
    else {
      // last argument
      retv.emplace_back(&text[p], text.size()-p);
    }
    p = x+1;

  } while (x != std::string::npos);

  return retv;
}
