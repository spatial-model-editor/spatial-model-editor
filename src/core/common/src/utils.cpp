#include "utils.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace sme {

namespace utils {

std::vector<std::string> toStdString(const QStringList &q) {
  std::vector<std::string> v;
  v.reserve(static_cast<std::size_t>(q.size()));
  std::transform(q.begin(), q.end(), std::back_inserter(v),
                 [](const QString &s) { return s.toStdString(); });
  return v;
}

QStringList toQString(const std::vector<std::string> &v) {
  QStringList q;
  q.reserve(static_cast<int>(v.size()));
  std::transform(v.begin(), v.end(), std::back_inserter(q),
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

constexpr std::size_t NULL_INDEX = std::numeric_limits<std::size_t>::max();

QPointFlattener::QPointFlattener(const QSize &boundingBox) : box(boundingBox) {}

bool QPointFlattener::isValid(const QPoint &point) const {
  bool xInside = (point.x() >= 0) && (point.x() < box.width());
  bool yInside = (point.y() >= 0) && (point.y() < box.height());
  return xInside && yInside;
}

std::size_t QPointFlattener::flatten(const QPoint &point) const {
  return static_cast<std::size_t>(point.x() * box.height() + point.y());
}

QPointIndexer::QPointIndexer(const QSize &boundingBox,
                             const std::vector<QPoint> &qPoints)
    : flattener(boundingBox),
      pointIndex(
          static_cast<std::size_t>(boundingBox.width() * boundingBox.height()),
          NULL_INDEX) {
  addPoints(qPoints);
}

void QPointIndexer::addPoints(const std::vector<QPoint> &qPoints) {
  for (const auto &point : qPoints) {
    if (!flattener.isValid(point)) {
      throw std::invalid_argument("invalid point: not within bounding box");
    }
    pointIndex[flattener.flatten(point)] = nPoints++;
  }
}

std::optional<std::size_t> QPointIndexer::getIndex(const QPoint &point) const {
  if (!flattener.isValid(point)) {
    return {};
  }
  auto index = pointIndex[flattener.flatten(point)];
  if (index == NULL_INDEX) {
    return {};
  }
  return index;
}

std::size_t QPointIndexer::getNumPoints() const { return nPoints; }

QPointUniqueIndexer::QPointUniqueIndexer(const QSize &boundingBox,
                                         const std::vector<QPoint> &qPoints)
    : flattener(boundingBox),
      pointIndex(
          static_cast<std::size_t>(boundingBox.width() * boundingBox.height()),
          NULL_INDEX) {
  addPoints(qPoints);
}

void QPointUniqueIndexer::addPoints(const std::vector<QPoint> &qPoints) {
  // add only unique points to vector & index
  for (const auto &point : qPoints) {
    if (!flattener.isValid(point)) {
      throw std::invalid_argument("invalid point: not within bounding box");
    }
    auto existingIndex = pointIndex[flattener.flatten(point)];
    if (existingIndex == NULL_INDEX) {
      pointIndex[flattener.flatten(point)] = nPoints++;
      points.push_back(point);
    }
  }
}

std::optional<std::size_t>
QPointUniqueIndexer::getIndex(const QPoint &point) const {
  if (!flattener.isValid(point)) {
    return {};
  }
  auto index = pointIndex[flattener.flatten(point)];
  if (index == NULL_INDEX) {
    return {};
  }
  return index;
}

std::vector<QPoint> QPointUniqueIndexer::getPoints() const { return points; }

} // namespace utils

} // namespace sme

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
