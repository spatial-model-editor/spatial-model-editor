// Python.h (included by pybind11.h) must come first
// https://docs.python.org/3.2/c-api/intro.html#include-files
#include <pybind11/pybind11.h>

#include "sme_exception.hpp"

namespace sme {

void pybindException(pybind11::module &m) {
  pybind11::register_exception<SmeRuntimeError>(m, "RuntimeError");
  pybind11::register_exception<SmeInvalidArgument>(m, "InvalidArgument");
}

} // namespace sme

//

//

//

//

//

//
