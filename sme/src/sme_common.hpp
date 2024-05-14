#pragma once

#include "sme/image_stack.hpp"
#include <QImage>
#include <algorithm>
#include <fmt/core.h>
#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <stdexcept>
#include <vector>

namespace pysme {

nanobind::ndarray<nanobind::numpy, std::uint8_t>
toPyImageRgb(const ::sme::common::ImageStack &imgs);
nanobind::ndarray<nanobind::numpy, bool>
toPyImageMask(const ::sme::common::ImageStack &imgs);

template <typename T> std::string vecToNames(const std::vector<T> &vec) {
  std::string str;
  for (const auto &elem : vec) {
    str.append(fmt::format("\n     - {}", elem.getName()));
  }
  return str;
}

template <typename T>
T &findElem(std::vector<T> &v, typename std::vector<T>::difference_type i) {
  if (i < 0) {
    i += static_cast<typename std::vector<T>::difference_type>(v.size());
  }
  auto idx{static_cast<std::size_t>(i)};
  if (i < 0 || idx >= v.size()) {
    throw std::invalid_argument(fmt::format("index {} out of bounds", i));
  }
  return v[idx];
}

template <typename T> T &findElem(std::vector<T> &v, const std::string &name) {
  if (auto iter = std::ranges::find_if(
          v, [&name](const auto &elem) { return elem.getName() == name; });
      iter != v.end()) {
    return *iter;
  }
  throw std::invalid_argument(fmt::format("name '{}' not found", name));
}

// a read-only iterable list of T with index or name-based lookup
template <typename T>
void bindList(nanobind::module_ &m, const char *typeName) {
  std::string listName{fmt::format("{}List", typeName)};
  std::string docString{
      fmt::format("a list of :class:`{1}` objects\n\n"
                  "the list can be iterated over, or an "
                  "element can be looked up by its index or name",
                  listName, typeName)};
  nanobind::class_<std::vector<T>>(m, listName.c_str(), docString.c_str())
      .def(nanobind::init<>())
      .def("__len__", [](const std::vector<T> &v) { return v.size(); })
      .def(
          "__getitem__",
          [](std::vector<T> &v, typename std::vector<T>::difference_type i)
              -> T & { return findElem(v, i); },
          nanobind::rv_policy::reference_internal)
      .def(
          "__getitem__",
          [](std::vector<T> &v, const std::string &name) -> T & {
            return findElem(v, name);
          },
          nanobind::rv_policy::reference_internal)
      .def(
          "__iter__",
          [](std::vector<T> &v) {
            return nanobind::make_iterator(
                nanobind::type<T>(), "sme_list_iterator", v.begin(), v.end());
          },
          nanobind::keep_alive<0, 1>());
}

// return the contents of a std::vector as a numpy ndarray with a given shape,
// without making a copy of the data, based on
// https://github.com/pybind/pybind11/issues/1042#issuecomment-642215028
template <typename UnderlyingType, typename Type = UnderlyingType>
static inline nanobind::ndarray<nanobind::numpy, Type>
as_ndarray(std::vector<UnderlyingType> &&v,
           std::initializer_list<std::size_t> shape) {
  auto data{v.data()};
  auto ptr = std::make_unique<std::vector<UnderlyingType>>(std::move(v));
  auto capsule = nanobind::capsule(ptr.get(), [](void *p) noexcept {
    std::unique_ptr<std::vector<UnderlyingType>>(
        static_cast<std::vector<UnderlyingType> *>(p));
  });
  ptr.release();
  return nanobind::ndarray<nanobind::numpy, Type>(data, shape, capsule);
}

template <typename UnderlyingType, typename Type = UnderlyingType>
static inline nanobind::ndarray<nanobind::numpy, Type>
as_ndarray(std::vector<UnderlyingType> &&v,
           const ::sme::common::Volume &shape) {
  return as_ndarray<UnderlyingType, Type>(
      std::move(v), {shape.depth(), static_cast<std::size_t>(shape.height()),
                     static_cast<std::size_t>(shape.width())});
}
} // namespace pysme
