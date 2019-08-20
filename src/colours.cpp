#include "colours.hpp"

namespace colours {

const QColor& indexedColours::operator[](std::size_t i) const {
  return colours[i % colours.size()];
}

}  // namespace colours
