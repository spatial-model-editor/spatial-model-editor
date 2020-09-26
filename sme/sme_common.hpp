#pragma once

#include "fmt/core.h"
#include <QImage>
#include <algorithm>
#include <exception>
#include <map>
#include <vector>

namespace sme {

using PyImage = std::vector<std::vector<std::vector<int>>>;
using PyConc = std::vector<std::vector<double>>;

PyImage toPyImage(const QImage &img);

PyConc toPyConc();

template <typename T> std::string vecToNames(const std::vector<T> &vec) {
  std::string str;
  for (const auto &elem : vec) {
    str.append(fmt::format("\n     - {}", elem.getName()));
  }
  return str;
}

template <typename T>
const T &findElem(const std::vector<T> &vec, const std::string &name,
                  const std::string &typeName) {
  if (auto iter =
          std::find_if(vec.cbegin(), vec.cend(),
                       [&name](const auto &v) { return v.getName() == name; });
      iter != vec.cend()) {
    return *iter;
  }
  throw std::invalid_argument(typeName + " '" + name + "' not found");
}

} // namespace sme
