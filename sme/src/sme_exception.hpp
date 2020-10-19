#pragma once

#include <pybind11/pybind11.h>
#include <stdexcept>
#include <string>

namespace sme {

void pybindException(pybind11::module &m);

class SmeRuntimeError : public std::runtime_error {
public:
  explicit SmeRuntimeError(const std::string &message)
      : std::runtime_error(message) {}
};

class SmeInvalidArgument : public std::invalid_argument {
public:
  explicit SmeInvalidArgument(const std::string &message)
      : std::invalid_argument(message) {}
};

} // namespace sme
