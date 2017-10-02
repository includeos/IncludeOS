#pragma once
#include <string>
#include <vector>

extern std::vector<std::string>
  ircsplit(const std::string& text);

extern std::vector<std::string>
  ircsplit(const std::string& text, std::string& source);
