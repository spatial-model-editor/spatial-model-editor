#pragma once

#include "sme/utils.hpp"
#include <string>
#include <string_view>

namespace sme::common {

/**
 * @brief Convert a user-visible name to a valid SBML-style SId.
 */
inline std::string nameToSId(std::string_view name,
                             std::string_view fallback = "_") {
  const std::string charsToConvertToUnderscore = " -_/";
  std::string id;
  // remove any non-alphanumeric chars, convert spaces etc to underscores
  for (auto c : name) {
    if (sme::common::isalnum(c)) {
      id.push_back(c);
    } else if (charsToConvertToUnderscore.find(c) != std::string::npos) {
      id.push_back('_');
    }
  }
  if (id.empty()) {
    id = fallback.empty() ? "_" : std::string{fallback};
  }
  // first char must be a letter or underscore
  if (!sme::common::isalpha(id.front()) && id.front() != '_') {
    id.insert(id.begin(), '_');
  }
  return id;
}

/**
 * @brief Create a unique id/name by appending a postfix until available.
 */
template <typename IsAvailable>
std::string makeUnique(std::string name, IsAvailable isAvailable,
                       std::string_view postfix = "_") {
  while (!isAvailable(name)) {
    name.append(postfix);
  }
  return name;
}

} // namespace sme::common
