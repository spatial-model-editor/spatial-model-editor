#include "catch_wrapper.hpp"
#include "logger.hpp"

SCENARIO("Logger", "[core/common/logger][core/common][core][logger]") {
  REQUIRE_NOTHROW(spdlog::info("Testing logger..."));
}
