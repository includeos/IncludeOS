
#ifndef UTIL_SIGNAL_HPP
#define UTIL_SIGNAL_HPP

#include <vector>
#include <delegate>

template <typename F>
class signal {
public:
  //! \brief Callable type of the signal handlers
  using handler = delegate<F>;

  //! \brief Default constructor
  explicit signal() = default;

  //! \brief Default destructor
  ~signal() noexcept = default;

  //! \brief Default move constructor
  explicit signal(signal&&) noexcept = default;

  //! \brief Default assignment operator
  signal& operator=(signal&&) = default;

  //! \brief Connect a callable object to this signal
  void connect(handler&& fn) {
    funcs.emplace_back(std::forward<handler>(fn));
  }

  //! \brief Emit this signal by executing all connected callable objects
  template<typename... Args>
  void emit(Args&&... args) {
    for(auto& fn : funcs)
      fn(std::forward<Args>(args)...);
  }

private:
  // Set of callable objects registered to be called on demand
  std::vector<handler> funcs;

  // Avoid copying
  signal(signal const&) = delete;

  // Avoid assignment
  signal& operator=(signal const&) = delete;
}; //< class signal

#endif //< UTILITY_SIGNAL_HPP
