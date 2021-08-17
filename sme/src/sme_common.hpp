#pragma once

#include "fmt/core.h"
#include "sme_exception.hpp"
#include <QImage>
#include <algorithm>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
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

} // namespace sme
