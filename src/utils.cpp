#include "utils.hpp"

namespace utils {

std::vector<std::string> toStdString(const QStringList& q) {
  std::vector<std::string> v;
  v.reserve(static_cast<std::size_t>(q.size()));
  std::transform(q.begin(), q.end(), std::back_inserter(v),
                 [](const QString& s) { return s.toStdString(); });
  return v;
}

QStringList toQString(const std::vector<std::string>& v) {
  QStringList q;
  q.reserve(static_cast<int>(v.size()));
  std::transform(v.begin(), v.end(), std::back_inserter(q),
                 [](const std::string& s) { return s.c_str(); });
  return q;
}

const QColor& indexedColours::operator[](std::size_t i) const {
  return colours[i % colours.size()];
}

}  // namespace utils
