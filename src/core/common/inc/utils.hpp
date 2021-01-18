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
#include <QRgb>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>
#include <array>
#include <initializer_list>
#include <iomanip>
#include <iterator>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sme {

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

template <typename Container>
std::size_t min_element_index(const Container &c) {
  return static_cast<std::size_t>(
      std::distance(c.cbegin(), std::min_element(c.cbegin(), c.cend())));
}

// https://stackoverflow.com/a/56766138
template <typename T> constexpr auto decltypeStr() {
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
QString dblToQStr(double x, int precision = 18);
std::vector<QRgb> toStdVec(const QVector<QRgb> &q);

std::vector<bool> toBool(const std::vector<int> &v);
std::vector<int> toInt(const std::vector<bool> &v);

template <typename T> std::vector<T> stringToVector(const std::string &str) {
  std::istringstream ss(str);
  return std::vector<T>(std::istream_iterator<T>(ss),
                        std::istream_iterator<T>{});
}

template <typename T> std::string vectorToString(const std::vector<T> &vec) {
  if (vec.empty()) {
    return {};
  }
  std::stringstream ss;
  for (std::size_t i = 0; i < vec.size() - 1; ++i) {
    ss << std::scientific << std::setprecision(17) << vec[i] << " ";
  }
  ss << vec.back();
  return ss.str();
}

template <typename T>
std::vector<std::size_t>
getIndicesOfSortedVector(const std::vector<T> &unsorted) {
  std::vector<std::size_t> indices(unsorted.size(), 0);
  std::iota(indices.begin(), indices.end(), 0);
  std::sort(indices.begin(), indices.end(),
            [&unsorted = unsorted](std::size_t i1, std::size_t i2) {
              return unsorted[i1] < unsorted[i2];
            });
  return indices;
}

class indexedColours {
private:
  const static std::vector<QColor> colours;

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

template <typename K, typename V> class SmallMap {
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
template <typename T, std::size_t MaxSize> class SmallStackSet {
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
  template <typename Cont> bool contains_any_of(const Cont &cont) const {
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

// erase elements [first, last) from v, treating v as cyclic
template <typename T>
void cyclicErase(std::vector<T> &v, std::size_t first, std::size_t last) {
  using diff = typename std::vector<T>::difference_type;
  if (first > last) {
    v.erase(v.begin() + static_cast<diff>(first), v.end());
    v.erase(v.begin(), v.begin() + static_cast<diff>(last));
    return;
  }
  v.erase(v.begin() + static_cast<diff>(first),
          v.begin() + static_cast<diff>(last));
}

template <typename T>
bool isCyclicPermutation(const std::vector<T> &a, const std::vector<T> &b) {
  if (a.size() != b.size()) {
    // different length: vectors not equal
    return false;
  }
  if (a.size() == 1) {
    // deal with single length vectors edge case
    return a[0] == b[0];
  }
  // find an element of b that matches the first element of a
  std::size_t ib{0};
  while (ib < b.size() && b[ib] != a.front()) {
    ++ib;
  }
  if (ib == b.size()) {
    // no element in b matches first element of a: vectors not equal
    return false;
  }
  // b[ib] == a[0]
  // by default iterate forwards through a & b
  bool fwd{true};
  if (a[1] == b[(ib + b.size() - 1) % b.size()]) {
    // next element of a equal to previous element of b: iterate backwards in b
    fwd = false;
  }
  for (const auto &elem : a) {
    if (elem != b[ib]) {
      // matching elements not equal: vectors not equal
      return false;
    }
    if (fwd) {
      ++ib;
      if (ib == b.size()) {
        ib = 0;
      }
    } else {
      if (ib == 0) {
        ib = b.size();
      }
      --ib;
    }
  }
  // all matching elements compare equal
  return true;
}

} // namespace utils

} // namespace sme
