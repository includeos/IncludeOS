#ifndef SERVER_REQUEST_HPP
#define SERVER_REQUEST_HPP

#include "http/inc/request.hpp"
#include "attribute.hpp"
#include <map>

namespace server {

class Request;
using Request_ptr = std::shared_ptr<Request>;

/**
 * @brief An extended HTTP Request.
 * @details Extends the basic HTTP Request by adding n attributes (Attribute)
 *
 */
class Request : public http::Request {
private:
  using Parent = http::Request;
  using buffer_t = std::shared_ptr<uint8_t>;

public:
  // inherit constructors
  using Parent::Parent;

  Request(buffer_t, size_t);

  /**
   * @brief Check if the given attribute exists.
   * @details Iterates over map and check if the given
   *
   * @tparam A : The specific attribute
   * @return : If the Request has the specific attribute.
   */
  template<typename A>
  bool has_attribute() const;

  /**
   * @brief Retrieve a shared ptr to the specific attribute.
   * @details Try to retrieve the specific attribute by looking up the type
   * as key inside the attribute map.
   *
   * @tparam A : The specific attribute
   * @return : A shared ptr to the specific attribute. (Can be null if not exists.)
   */
  template<typename A>
  std::shared_ptr<A> get_attribute();

  /**
   * @brief Add/set a specific attribute.
   * @details Inserts a shared ptr of the specific attribute with type as key. (Will replace if already exists)
   *
   * @param  : A shared ptr to the specific attribute
   * @tparam A : The specific attribute
   */
  template<typename A>
  void set_attribute(std::shared_ptr<A>);

private:
  /**
   * @brief A map with pointers to attributes.
   * @details A map with a unique key to a specific attribute
   * and a pointer to the base class Attribute.
   * (Since we got more than one request, an Attribute can't be static)
   */
  std::map<AttrType, Attribute_ptr> attributes_;

}; // < server::Request

template<typename A>
bool Request::has_attribute() const {
  return attributes_.find(Attribute::type<A>()) != attributes_.end();
}

template<typename A>
std::shared_ptr<A> Request::get_attribute() {
  auto it = attributes_.find(Attribute::type<A>());
  if(it != attributes_.end())
    return std::static_pointer_cast<A>(it->second);
  return nullptr;
}

template<typename A>
void Request::set_attribute(std::shared_ptr<A> attr) {
  attributes_.insert({Attribute::type<A>(), attr});
}

}; // < namespace server



#endif
