#include "catch_wrapper.hpp"
#include "logger.hpp"

TEST_CASE("Logger", "[core/common/logger][core/common][core][logger]") {
  REQUIRE_NOTHROW(spdlog::info("Testing logger..."));
}
