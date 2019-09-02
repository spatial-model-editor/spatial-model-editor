#include "utils.hpp"

namespace utils {

const QColor& indexedColours::operator[](std::size_t i) const {
  return colours[i % colours.size()];
}

}
