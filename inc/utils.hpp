// utilities
//  - decltypeStr<T>: type of T as a string
//    (taken from https://stackoverflow.com/a/56766138)
//  - toStdString: convert QStringList to std::vector<std::string>
//  - toQString: convert std::vector<std::string> to QStringList
//  - stringToVector: convert space-delimited list of values to a vector
//  - vectorToString: convert vector of values to a space-delimited list
//  - indexedColours: a set of default colours for display purposes
//  - QPointIndexer class:
//     - given vector of QPoints
//     - lookup index of any QPoint in the vector
//     - returns std::optional with index if found
//  - QPointUniqueIndexer class:
//     - as above but removes duplicated QPoints first

#pragma once

#include <QColor>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QStringList>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <vector>

namespace utils {

template <typename T>
constexpr auto decltypeStr() {
  std::string_view name;
  std::string_view prefix;
  std::string_view suffix;
#ifdef __clang__
  name = __PRETTY_FUNCTION__;
  prefix = "auto type_name() [T = ";
  suffix = "]";
#elif defined(__GNUC__)
  name = __PRETTY_FUNCTION__;
  prefix = "constexpr auto type_name() [with T = ";
  suffix = "]";
#elif defined(_MSC_VER)
  name = __FUNCSIG__;
  prefix = "auto __cdecl type_name<";
  suffix = ">(void)";
#endif
  name.remove_prefix(prefix.size());
  name.remove_suffix(suffix.size());
  return name;
}

std::vector<std::string> toStdString(const QStringList &q);
QStringList toQString(const std::vector<std::string> &v);

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

class QPointIndexer {
 protected:
  QSize box;
  std::size_t nPoints;
  std::vector<std::size_t> pointIndex;
  bool validQPoint(const QPoint &point) const;
  std::size_t flattenQPoint(const QPoint &point) const;

 public:
  explicit QPointIndexer(const QSize &boundingBox,
                         const std::vector<QPoint> &qPoints = {});
  void addPoints(const std::vector<QPoint> &qPoints);
  std::optional<std::size_t> getIndex(const QPoint &point) const;
};

class QPointUniqueIndexer : public QPointIndexer {
 protected:
  std::vector<QPoint> points;
  void init(const std::vector<QPoint> &qPoints);

 public:
  explicit QPointUniqueIndexer(const QSize &boundingBox,
                               const std::vector<QPoint> &qPoints = {});
  void addPoints(const std::vector<QPoint> &qPoints);
  std::vector<QPoint> getPoints() const;
};

}  // namespace utils
