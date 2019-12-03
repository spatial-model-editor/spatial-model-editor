// catch2 header include with
// - dbl_approx() convenience function for Approx(x).epsilon(1e-14)

#pragma once

#define CATCH_CONFIG_FAST_COMPILE
#define CATCH_CONFIG_DISABLE_MATCHERS

#include <catch.hpp>

// add custom Approximation convenience function for doubles
inline Catch::Detail::Approx dbl_approx(double x) {
  return Catch::Detail::Approx(x).epsilon(1e-14);
}
