#pragma once

#include "fmt/core.h"
#include <QImage>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace sme {
using PyImageRgb = std::vector<std::vector<std::vector<int>>>;
using PyImageMask = std::vector<std::vector<bool>>;
using PyConc = std::vector<std::vector<double>>;

PyImageRgb toPyImageRgb(const QImage &img);
PyImageMask toPyImageMask(const QImage &img);
PyConc toPyConc();

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
  throw SmeInvalidArgument(typeName + " '" + name + "' not found");
}

} // namespace sme
