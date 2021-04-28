#pragma once

#include "sme_model.hpp"
#include <pybind11/pybind11.h>

namespace sme {

void pybindModule(pybind11::module &m);

Model openFile(const std::string &filename);
Model openSbmlFile(const std::string &filename);
Model openExampleModel();

} // namespace sme
