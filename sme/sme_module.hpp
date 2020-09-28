#pragma once
#include "sme_model.hpp"

namespace pybind11 {
class module;
}

namespace sme {

void pybindModule(pybind11::module &m);

Model openSbmlFile(const std::string &filename);
Model openExampleModel();

} // namespace sme
