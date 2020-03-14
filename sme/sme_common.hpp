#pragma once

#include <map>
#include <vector>

#include "fmt/core.h"

template <typename T>
std::string vecToNames(const std::vector<T>& vec) {
  std::string str;
  for (const auto& elem : vec) {
    str.append(fmt::format("\n     - {}", elem.getName()));
  }
  return str;
}

template <typename T>
std::map<std::string, T*> vecToNamePtrMap(std::vector<T>& vec) {
  std::map<std::string, T*> m;
  for (auto& elem : vec) {
    m.insert({elem.getName(), &elem});
  }
  return m;
}
