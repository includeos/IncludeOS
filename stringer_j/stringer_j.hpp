#ifndef STRINGER_J_HPP
#define STRINGER_J_HPP

#include <sstream>
#include <vector>

namespace stringerj {

/**
 * @brief Stringer Bell's long lost brother
 * @details Builds values into a JSON string object
 *
 */
class StringerJ {

public:

  StringerJ();

  template<typename T>
  StringerJ& add(std::string field, T val);

  template<typename T>
  static std::string array(const std::string&, const std::vector<T>&);

  std::string str();

private:
  std::ostringstream builder;

  StringerJ& open();
  StringerJ& close();


};

template<typename T>
StringerJ& StringerJ::add(std::string field, T val) {
  builder << " \"" << field << "\""
    << ":" << "\"" << val << "\"" << ",";

  return *this;
}

template<typename T>
std::string StringerJ::array(const std::string& name, const std::vector<T>& vec) {
  std::ostringstream os;
  os << name << ":[ ";
  for(auto val : vec)
    os << val << ",";
  os.seekp(-1, os.cur);
  os << " ]";
  return os.str();
}


}; // < namespace stringer_j

#endif
