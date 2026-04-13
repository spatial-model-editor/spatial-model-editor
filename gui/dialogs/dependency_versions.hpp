#pragma once

#include "sme/version.hpp"
#include <vector>

namespace sme::gui {

[[nodiscard]] std::vector<sme::common::DependencyVersion>
getGuiDependencyVersions();

} // namespace sme::gui
