#pragma once

#include "sme_model.hpp"
#include <nanobind/nanobind.h>

namespace pysme {

void bindModule(nanobind::module_ &m);

Model openFile(const std::string &filename);
Model openSbmlFile(const std::string &filename);
Model openExampleModel(const std::string &name);

} // namespace pysme
