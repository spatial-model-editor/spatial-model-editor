#pragma once

#include "fmt/core.h"
#include "sme_exception.hpp"
#include <QImage>
#include <algorithm>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <stdexcept>
#include <vector>

namespace sme {

pybind11::array toPyImageRgb(const QImage &img);
pybind11::array toPyImageMask(const QImage &img);

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
    throw sme::SmeInvalidArgument(fmt::format("index {} out of bounds", i));
  }
  return v[idx];
}

template <typename T> T &findElem(std::vector<T> &v, const std::string &name) {
  if (auto iter = std::find_if(
          v.begin(), v.end(),
          [&name](const auto &elem) { return elem.getName() == name; });
      iter != v.end()) {
    return *iter;
  }
  throw SmeInvalidArgument(fmt::format("name '{}' not found", name));
}

// a read-only iterable list of T with index or name-based lookup
template <typename T> void bindList(pybind11::module &m, const char *typeName) {
  std::string listName{fmt::format("{}List", typeName)};
  std::string docString{
      fmt::format("a list of :class:`{1}` objects\n\n"
                  "the list can be iterated over, or an "
                  "element can be looked up by its index or name",
                  listName, typeName)};
  pybind11::class_<std::vector<T>>(m, listName.c_str(), docString.c_str())
      .def(pybind11::init<>())
      .def("__len__", [](const std::vector<T> &v) { return v.size(); })
      .def(
          "__getitem__",
          [](std::vector<T> &v, typename std::vector<T>::difference_type i)
              -> T & { return sme::findElem(v, i); },
          pybind11::return_value_policy::reference_internal)
      .def(
          "__getitem__",
          [](std::vector<T> &v, const std::string &name) -> T & {
            return sme::findElem(v, name);
          },
          pybind11::return_value_policy::reference_internal)
      .def(
          "__iter__",
          [](std::vector<T> &v) {
            return pybind11::make_iterator(v.begin(), v.end());
          },
          pybind11::keep_alive<0, 1>());
}

// return the contents of a std::vector as an ndarray, with optional shape,
// without making a copy of the data, based on
// https://github.com/pybind/pybind11/issues/1042#issuecomment-642215028
template <typename T>
static inline pybind11::array_t<T>
as_ndarray(std::vector<T> &&v, const std::vector<ssize_t> &shape = {}) {
  auto data{v.data()};
  auto size{static_cast<ssize_t>(v.size())};
  auto ptr{std::make_unique<std::vector<T>>(std::move(v))};
  auto capsule{pybind11::capsule(ptr.get(), [](void *p) {
    std::unique_ptr<std::vector<T>>(reinterpret_cast<std::vector<T> *>(p));
  })};
  ptr.release();
  if (shape.empty()) {
    return pybind11::array(size, data, capsule);
  }
  return pybind11::array(shape, data, capsule);
}

} // namespace sme
