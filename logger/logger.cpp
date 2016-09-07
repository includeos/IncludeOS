
#include "logger.hpp"
#include <sstream>
#include <common>
//#include <info>

Logger::Logger(Log& log, Log::index_type pos)
: log_(log), pos_{&log_, pos}
{
}

void Logger::log(const std::string& str) {

  if(UNLIKELY( str.empty() ))
    return;

  if(UNLIKELY( str.size() + 1 >= log_.size() ))
  {
    // start at the beginning
    pos_ = {&log_, 0};
    // calculate offset (with padding)
    auto offset = str.size() + 1 - log_.size();
    // copy later part into position
    std::copy(str.begin() + offset, str.end(), pos_);
    // increment position to know where to put padding
    pos_ += str.size() - offset;
    *pos_ = '\0';
    // return to the beginning
    ++pos_;
    return;
  }

  std::copy(str.begin(), str.end(), pos_);
  pos_ += str.size();

  // add null terminate padding
  auto it = pos_;
  while(*it != '\0') {
    *it = '\0';
    ++it;
  }
  ++pos_;
}

std::vector<std::string> Logger::entries(size_t n) const {
  std::vector<std::string> results;

  if(n) results.reserve(n);

  auto head = pos_;

  do {
    // adjust head
    while(*head == '\0') {
      ++head;
      if(head == pos_) return results;
    }

    std::ostringstream ss;

    while(*head != '\0') {
      ss << *head;
      ++head;
    }

    results.emplace_back(std::forward<std::string>(ss.str()));
    n--;

    /*// setup tail
    auto tail = head;

    while(*tail != '\0')
      ++tail;

    results.emplace_back(head, tail);
    head = tail;*/

  } while(n != 0);

  return results;
}

void Logger::flush() {
  std::fill(log_.begin(), log_.end(), 0);
  pos_ = {&log_, 0};
}
