#include "catch_wrapper.hpp"
#include "sme/id.hpp"
#include <set>

using namespace sme;

TEST_CASE("Common id", "[core/common/id][core/common][core][id]") {
  SECTION("nameToSId") {
    REQUIRE(common::nameToSId("valid_id") == "valid_id");
    REQUIRE(common::nameToSId("name with spaces") == "name_with_spaces");
    REQUIRE(common::nameToSId("name-with-spaces") == "name_with_spaces");
    REQUIRE(common::nameToSId("name/with/spaces") == "name_with_spaces");
    REQUIRE(common::nameToSId("!? invalid / Chars!!-") == "_invalid___Chars_");
    REQUIRE(common::nameToSId("1name") == "_1name");
    REQUIRE(common::nameToSId("!@#", "feature") == "feature");
  }
  SECTION("makeUnique") {
    const std::set<std::string> names{"ab", "abc", "_ab", "ab_"};
    auto isAvailable = [&names](const std::string &name) {
      return !names.contains(name);
    };
    REQUIRE(common::makeUnique("a", isAvailable, "_") == "a");
    REQUIRE(common::makeUnique("ab", isAvailable, "_") == "ab__");
    REQUIRE(common::makeUnique("ab", isAvailable, "1") == "ab1");
    REQUIRE(common::makeUnique("abc", isAvailable, "_") == "abc_");
  }
}
