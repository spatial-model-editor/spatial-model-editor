// utilities
//  - stringToVector: convert space-delimited list of values to a vector
//  - vectorToString: convert vector of values to a space-delimited list
//  - indexedColours: a set of default colours for display purposes

#pragma once

#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>
#include <QColor>

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

class indexedColours {
 private:
  const std::vector<QColor> colours{
      {230, 25, 75},  {60, 180, 75},   {255, 225, 25}, {0, 130, 200},
      {245, 130, 48}, {145, 30, 180},  {70, 240, 240}, {240, 50, 230},
      {210, 245, 60}, {250, 190, 190}, {0, 128, 128},  {230, 190, 255},
      {170, 110, 40}, {255, 250, 200}, {128, 0, 0},    {170, 255, 195},
      {128, 128, 0},  {255, 215, 180}, {0, 0, 128},    {128, 128, 128}};

 public:
  const QColor &operator[](std::size_t i) const;
};

}  // namespace utils
