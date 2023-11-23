#include "sme/utils.hpp"
#include "sme/image_stack.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace sme::common {

std::vector<std::string> toStdString(const QStringList &q) {
  std::vector<std::string> v;
  v.reserve(static_cast<std::size_t>(q.size()));
  std::ranges::transform(q, std::back_inserter(v),
                         [](const QString &s) { return s.toStdString(); });
  return v;
}

QStringList toQString(const std::vector<std::string> &v) {
  QStringList q;
  q.reserve(static_cast<int>(v.size()));
  std::ranges::transform(v, std::back_inserter(q),
                         [](const std::string &s) { return s.c_str(); });
  return q;
}

QString dblToQStr(double x, int precision) {
  return QString::number(x, 'g', precision);
}

std::vector<QRgb> toStdVec(const QVector<QRgb> &q) {
  std::vector<QRgb> v;
  v.reserve(static_cast<std::size_t>(q.size()));
  for (auto c : q) {
    v.push_back(c);
  }
  return v;
}

std::vector<bool> toBool(const std::vector<int> &v) {
  std::vector<bool> r;
  r.reserve(v.size());
  for (auto i : v) {
    r.push_back(i == 1);
  }
  return r;
}

std::vector<int> toInt(const std::vector<bool> &v) {
  std::vector<int> r;
  r.reserve(v.size());
  for (auto b : v) {
    if (b) {
      r.push_back(1);
    } else {
      r.push_back(0);
    }
  }
  return r;
}

const std::vector<QColor> indexedColours::colours = std::vector<QColor>{
    {230, 25, 75},  {60, 180, 75},   {255, 225, 25}, {0, 130, 200},
    {245, 130, 48}, {145, 30, 180},  {70, 240, 240}, {240, 50, 230},
    {210, 245, 60}, {250, 190, 190}, {0, 128, 128},  {230, 190, 255},
    {170, 110, 40}, {255, 250, 200}, {128, 0, 0},    {170, 255, 195},
    {128, 128, 0},  {255, 215, 180}, {0, 0, 128},    {128, 128, 128}};

const QColor &indexedColours::operator[](std::size_t i) const {
  return colours[i % colours.size()];
}

} // namespace sme::common

// extra lines to work around sonarsource/coverage bug

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//

//
