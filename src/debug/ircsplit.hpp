#include <vector>

std::vector<std::string>
split(const std::string& text, std::string& source)
{
  std::vector<std::string> retv;
  size_t x = 0;
  size_t p = 0;
  // ignore empty messages
  if (text.empty()) return retv;
  // check for source
  if (text[0] == ':')
  {
    x = text.find(" ", 1);
    source = text.substr(x);
    // early return for source-only msg
    if (x == std::string::npos) return retv;
    p = x+1;
  }
  // parse remainder
  do
  {
    x = text.find(" ", p+1);
    size_t y = text.find(":", x+1); // find last param
    
    if (y == x+1)
    {
      // single argument
      retv.push_back(text.substr(p, x-p));
      // ending text argument
      retv.push_back(text.substr(y+1));
      break;
    }
    else if (x != std::string::npos)
    {
      // single argument
      retv.push_back(text.substr(p, x-p));
    }
    else
    {
      // last argument
      retv.push_back(text.substr(p));
    }
    p = x+1;
    
  } while (x != std::string::npos);
  
  return retv;
}
