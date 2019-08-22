// misc utilities
//  - stringToVector: convert space-delimited list of values to a vector
//  - vectorToString: convert vector of values to a space-delimited list

#pragma once

#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

namespace utils {

template <class T>
std::vector<T> stringToVector(const std::string &str) {
  std::istringstream ss(str);
  return std::vector<T>(std::istream_iterator<T>(ss),
                        std::istream_iterator<T>{});
}

template <class T>
std::string vectorToString(const std::vector<T> &vec) {
  std::stringstream ss;
  for (std::size_t i = 0; i < vec.size() - 1; ++i) {
    ss << std::scientific << std::setprecision(17) << vec[i] << " ";
  }
  ss << vec.back();
  return ss.str();
}

}  // namespace utils
