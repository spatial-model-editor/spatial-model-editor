#include "utils.hpp"

#include <stdexcept>

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

const QColor &indexedColours::operator[](std::size_t i) const {
  return colours[i % colours.size()];
}

constexpr std::size_t NULL_INDEX = std::numeric_limits<std::size_t>::max();

bool QPointIndexer::validQPoint(const QPoint &point) const {
  bool xInside = (point.x() >= 0) && (point.x() < box.width());
  bool yInside = (point.y() >= 0) && (point.y() < box.height());
  return xInside && yInside;
}

std::size_t QPointIndexer::flattenQPoint(const QPoint &point) const {
  return static_cast<std::size_t>(point.x() * box.height() + point.y());
}

void QPointIndexer::addPoints(const std::vector<QPoint> &qPoints) {
  for (const auto &point : qPoints) {
    if (!validQPoint(point)) {
      throw std::invalid_argument("invalid point: not within bounding box");
    }
    pointIndex[flattenQPoint(point)] = nPoints++;
  }
}

QPointIndexer::QPointIndexer(const QSize &boundingBox,
                             const std::vector<QPoint> &qPoints)
    : box(boundingBox),
      nPoints(0),
      pointIndex(
          static_cast<std::size_t>(boundingBox.width() * boundingBox.height()),
          NULL_INDEX) {
  addPoints(qPoints);
}

std::optional<std::size_t> QPointIndexer::getIndex(const QPoint &point) const {
  if (!validQPoint(point)) {
    return {};
  }
  auto index = pointIndex[flattenQPoint(point)];
  if (index == NULL_INDEX) {
    return {};
  }
  return index;
}

QPointUniqueIndexer::QPointUniqueIndexer(const QSize &boundingBox,
                                         const std::vector<QPoint> &qPoints)
    : QPointIndexer(boundingBox) {
  addPoints(qPoints);
}

void QPointUniqueIndexer::addPoints(const std::vector<QPoint> &qPoints) {
  // add only unique points to vector & index
  for (const auto &point : qPoints) {
    if (!validQPoint(point)) {
      throw std::invalid_argument("invalid point: not within bounding box");
    }
    auto existingIndex = pointIndex[flattenQPoint(point)];
    if (existingIndex == NULL_INDEX) {
      pointIndex[flattenQPoint(point)] = nPoints++;
      points.push_back(point);
    }
  }
}

std::vector<QPoint> QPointUniqueIndexer::getPoints() const { return points; }

}  // namespace utils
