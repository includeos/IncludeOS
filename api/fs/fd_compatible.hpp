
#pragma once
#ifndef INCLUDE_FD_COMPATIBLE_HPP
#define INCLUDE_FD_COMPATIBLE_HPP

#include <posix/fd.hpp>
#include <delegate>

/**
 * @brief      Makes classes inheriting this carry the
 *             attribute to be able to create a
 *             file descriptor of the resource.
 */
class FD_compatible {
public:
  delegate<FD&()> open_fd = nullptr;

  virtual ~FD_compatible() = default;

};

#endif
