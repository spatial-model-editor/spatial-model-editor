#include "catch_wrapper.hpp"
#include "sme/version.hpp"
#include <algorithm>
#include <string_view>

TEST_CASE("Core dependency versions are available",
          "[core/common][common][version]") {
  const auto dependencies = sme::common::getCoreDependencyVersions();
  REQUIRE_FALSE(dependencies.empty());

  const auto hasDependency = [&](std::string_view name) {
    return std::any_of(
        dependencies.cbegin(), dependencies.cend(),
        [name](const auto &dependency) { return dependency.name == name; });
  };

  REQUIRE(hasDependency("libSBML"));
  REQUIRE(hasDependency("libCombine"));
  REQUIRE(hasDependency("Qt"));
}
