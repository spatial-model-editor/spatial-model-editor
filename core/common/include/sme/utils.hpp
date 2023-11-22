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
//  - SmallMap: simple insert-only map for small number of small types
//  - SmallStackSet: simple fast non-allocating set implementation for a small

#pragma once

#include <QColor>
#include <QImage>
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

namespace sme::common {

/**
 * @brief The sum of all elements in a container
 */
template <typename Container>
typename Container::value_type sum(const Container &c) {
  return (std::accumulate(std::cbegin(c), std::cend(c),
                          typename Container::value_type{0}));
}

/**
 * @brief The average of all elements in a container
 */
template <typename Container>
typename Container::value_type average(const Container &c) {
  return sum(c) / static_cast<typename Container::value_type>(c.size());
}

/**
 * @brief The minimum value in a container
 */
template <typename Container>
typename Container::value_type min(const Container &c) {
  return *std::min_element(std::cbegin(c), std::cend(c));
}

/**
 * @brief The maximum value in a container
 */
template <typename Container>
typename Container::value_type max(const Container &c) {
  return *std::max_element(std::cbegin(c), std::cend(c));
}

/**
 * @brief The unique values from a container
 */
template <typename Container>
std::vector<typename Container::value_type>
get_unique_values(const Container &c) {
  std::vector<typename Container::value_type> uniq_values(c);
  std::sort(uniq_values.begin(), uniq_values.end());
  uniq_values.erase(std::unique(uniq_values.begin(), uniq_values.end()),
                    uniq_values.end());
  return uniq_values;
}

/**
 * @brief Are the numbers in the container indexes?
 */
template <typename Container>
bool isItIndexes(const Container &c, std::size_t length) {
  auto vec1 = common::get_unique_values(c);
  std::vector<typename Container::value_type> vec2(length);
  std::iota(std::begin(vec2), std::end(vec2), 0);
  return vec1 == vec2;
}

/**
 * @brief The minimum and maximum values in a container
 */
template <typename Container>
std::pair<typename Container::value_type, typename Container::value_type>
minmax(const Container &c) {
  auto p = std::minmax_element(std::cbegin(c), std::cend(c));
  return {*p.first, *p.second};
}

/**
 * @brief The index in the container of the minimum element
 */
template <typename Container>
std::size_t min_element_index(const Container &c) {
  return static_cast<std::size_t>(
      std::distance(c.cbegin(), std::min_element(c.cbegin(), c.cend())));
}

/**
 * @brief The index in the container of the matching element
 */
template <typename Container, typename Element>
std::size_t element_index(const Container &c, const Element &e,
                          std::size_t index_if_not_found = 0) {
  auto iter{std::find(cbegin(c), cend(c), e)};
  if (iter == cend(c)) {
    return index_if_not_found;
  }
  return static_cast<std::size_t>(std::distance(cbegin(c), iter));
}

/**
 * @brief The type of an object as a string
 *
 * @note Copied from https://stackoverflow.com/a/56766138
 */
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

/**
 * @brief Convert a QStringList to a vector of std::string
 */
std::vector<std::string> toStdString(const QStringList &q);

/**
 * @brief Convert a vector of std::string to a QStringList
 */
QStringList toQString(const std::vector<std::string> &v);

/**
 * @brief Convert a double to a QString
 */
QString dblToQStr(double x, int precision = 18);

/**
 * @brief Convert a QVector of QRgb to a vector of QRgb
 */
std::vector<QRgb> toStdVec(const QVector<QRgb> &q);

/**
 * @brief Convert a vector of `int` to a vector of `bool`
 */
std::vector<bool> toBool(const std::vector<int> &v);

/**
 * @brief Convert a vector of `bool` to a vector of `int`
 */
std::vector<int> toInt(const std::vector<bool> &v);

/**
 * @brief Convert a string to a vector of values
 *
 * The values in the string are separated by spaces.
 *
 * @tparam T the type of the values
 */
template <typename T> std::vector<T> stringToVector(const std::string &str) {
  std::istringstream ss(str);
  return std::vector<T>(std::istream_iterator<T>(ss),
                        std::istream_iterator<T>{});
}

/**
 * @brief Convert a vector of values to a string
 *
 * The values in the string are separated by spaces.
 * Doubles are printed in scientific notation with 17 significant digits.
 *
 * @tparam T the type of the values
 */
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

/**
 * @brief Indices of elements if vector were sorted
 *
 * Returns a vector of indices, such that the vector would be sorted if the
 * elements were in this order.
 *
 * @tparam T the type of the values in the vector
 */
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

/**
 * @brief Default set of colours
 *
 * A vector of default colours
 */
class indexedColours {
private:
  const static std::vector<QColor> colours;

public:
  const QColor &operator[](std::size_t i) const;
};

/**
 * @brief Cyclic erase of elements from a vector
 *
 * Erase the elements with index [first, last) from the vector v,
 * with cyclic indices, i.e. the next element after the last element is the
 * first element of the vector
 *
 * @tparam T the value type
 */
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

/**
 * @brief Check if a vector is a cyclic permutation of another vector
 *
 * @note Assumes the elements of the vector are unique
 *
 * @tparam T the value type
 */
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

// Creates a unique_ptr of type T with std::free as custom deleter
// Avoids taking this address of std::free as this is undefined behaviour
// https://stackoverflow.com/questions/27440953/stdunique-ptr-for-c-functions-that-need-free/43626234#43626234
struct free_deleter {
  template <typename T> void operator()(T *p) const {
    std::free(const_cast<std::remove_const_t<T> *>(p));
  }
};
template <typename T> using unique_C_ptr = std::unique_ptr<T, free_deleter>;

} // namespace sme::common
