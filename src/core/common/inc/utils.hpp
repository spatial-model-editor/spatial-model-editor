// utilities
//  - sum/average/min/max: sum/average/min/max of a container of values
//  - minmax: pair of min,max values in a container
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
//  - SmallMap: simple insert-only map for small number of small types
//  - SmallStackSet: simple fast non-allocating set implementation for a small
//  number of small (i.e. pass by copy) types, with hard limit on size

#pragma once

#include <QColor>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <array>
#include <initializer_list>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <vector>

namespace utils {

template <typename Container>
typename Container::value_type sum(const Container &c) {
  return (std::accumulate(std::cbegin(c), std::cend(c),
                          typename Container::value_type{0}));
}

template <typename Container>
typename Container::value_type average(const Container &c) {
  return sum(c) / static_cast<typename Container::value_type>(c.size());
}

template <typename Container>
typename Container::value_type min(const Container &c) {
  return *std::min_element(std::cbegin(c), std::cend(c));
}

template <typename Container>
typename Container::value_type max(const Container &c) {
  return *std::max_element(std::cbegin(c), std::cend(c));
}

template <typename Container>
std::pair<typename Container::value_type, typename Container::value_type>
minmax(const Container &c) {
  auto p = std::minmax_element(std::cbegin(c), std::cend(c));
  return {*p.first, *p.second};
}

// https://stackoverflow.com/a/56766138
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
std::vector<QRgb> toStdVec(const QVector<QRgb> &q);

template <typename T>
std::vector<T> stringToVector(const std::string &str) {
  std::istringstream ss(str);
  return std::vector<T>(std::istream_iterator<T>(ss),
                        std::istream_iterator<T>{});
}

template <typename T>
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

class QPointFlattener {
 private:
  QSize box;

 public:
  explicit QPointFlattener(const QSize &boundingBox);
  bool isValid(const QPoint &point) const;
  std::size_t flatten(const QPoint &point) const;
};

class QPointIndexer {
 private:
  QPointFlattener flattener;
  std::size_t nPoints = 0;
  std::vector<std::size_t> pointIndex;

 public:
  explicit QPointIndexer(const QSize &boundingBox,
                         const std::vector<QPoint> &qPoints = {});
  void addPoints(const std::vector<QPoint> &qPoints);
  std::optional<std::size_t> getIndex(const QPoint &point) const;
  std::size_t getNumPoints() const;
};

class QPointUniqueIndexer {
 private:
  QPointFlattener flattener;
  std::size_t nPoints = 0;
  std::vector<std::size_t> pointIndex;
  std::vector<QPoint> points;

 public:
  explicit QPointUniqueIndexer(const QSize &boundingBox,
                               const std::vector<QPoint> &qPoints = {});
  void addPoints(const std::vector<QPoint> &qPoints);
  std::optional<std::size_t> getIndex(const QPoint &point) const;
  std::vector<QPoint> getPoints() const;
};

template <typename K, typename V>
class SmallMap {
 private:
  std::vector<K> keys;
  std::vector<V> values;

 public:
  void insert(K key, V value) noexcept {
    keys.push_back(key);
    values.push_back(value);
  }
  std::optional<V> operator[](K key) const noexcept {
    for (std::size_t i = 0; i < keys.size(); ++i) {
      if (keys[i] == key) {
        return values[i];
      }
    }
    return {};
  }
  explicit SmallMap(std::size_t size) {
    keys.reserve(size);
    values.reserve(size);
  }
};

// set class for a fixed maximum number of small (i.e. pass by value) types
// no heap allocations, elements stored in std::array,
// insert/erase/find operations involve linear traversal of elements i.e. O(N)
// if the set is full then insert just becomes a no-op
template <typename T, std::size_t MaxSize>
class SmallStackSet {
 private:
  using container = std::array<T, MaxSize>;
  using const_iterator = typename container::const_iterator;
  container values;
  std::size_t n = 0;

 public:
  using value_type = T;
  void clear() noexcept { n = 0; }
  void insert(T v) {
    if (n == MaxSize || contains(v)) {
      return;
    }
    values[n] = v;
    ++n;
  }
  void erase(T v) {
    for (std::size_t i = 0; i < n; ++i) {
      if (values[i] == v) {
        --n;
        values[i] = values[n];
        return;
      }
    }
  }
  bool contains(T v) const {
    for (std::size_t i = 0; i < n; ++i) {
      if (values[i] == v) {
        return true;
      }
    }
    return false;
  }
  template <typename Cont>
  bool contains_any_of(const Cont &cont) const {
    return std::any_of(std::cbegin(cont), std::cend(cont),
                       [this](T v) { return contains(v); });
  }
  T operator[](std::size_t i) const { return values[i]; }
  const_iterator cbegin() const noexcept { return values.cbegin(); }
  const_iterator cend() const noexcept { return values.cbegin() + n; }
  const_iterator begin() const noexcept { return cbegin(); }
  const_iterator end() const noexcept { return cend(); }
  std::size_t size() const { return n; }
  std::size_t max_size() const { return MaxSize; }
  SmallStackSet() = default;
  explicit SmallStackSet(T v) { insert(v); }
  explicit SmallStackSet(std::initializer_list<T> vals) {
    for (T v : vals) {
      insert(v);
    }
  }
};
}  // namespace utils
