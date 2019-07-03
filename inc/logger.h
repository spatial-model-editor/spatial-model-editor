// Logger class
//  - include spdlog (https://github.com/gabime/spdlog)
//  - define operator<< overloads for some custom types

#pragma once

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <iostream>

#include "spdlog/spdlog.h"

template <class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
  os << "{ ";
  for (const auto& v : vec) {
    os << v << " ";
  }
  return os << "}";
}

#include "spdlog/fmt/ostr.h"
