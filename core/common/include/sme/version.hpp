#pragma once

#include <string>
#include <vector>

namespace sme::common {

/**
 * @brief Project version string (semantic version).
 */
extern const char *const SPATIAL_MODEL_EDITOR_VERSION;

struct DependencyVersion {
  std::string name;
  std::string url;
  std::string version;
};

[[nodiscard]] std::vector<DependencyVersion> getCoreDependencyVersions();

} // namespace sme::common
