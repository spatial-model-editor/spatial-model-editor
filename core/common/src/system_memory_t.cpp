#include "catch_wrapper.hpp"
#include "sme/system_memory.hpp"

using namespace sme;

TEST_CASE("System memory info", "[core/common/system_memory][core][common]") {
  const auto info = common::getSystemMemoryInfo();
  REQUIRE(info.has_value());
  REQUIRE(info->totalPhysicalBytes > 0);
  REQUIRE(info->availablePhysicalBytes > 0);
  REQUIRE(info->availablePhysicalBytes <= info->totalPhysicalBytes);

  const auto residentBytes = common::getCurrentProcessMemoryUsageBytes();
  REQUIRE(residentBytes.has_value());
  REQUIRE(residentBytes.value() > 0);
}
