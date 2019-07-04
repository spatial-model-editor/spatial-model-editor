// catch2 header include with
// - additional ostream operator << overloads for some Qt types
// - dbl_approx() convenience function for Approx(x).epsilon(1e-14)

#pragma once

#include "ext/catch/catch.hpp"
#include "ostream_overloads.hpp"

// add custom Approximation convenience function for doubles
inline Catch::Detail::Approx dbl_approx(double x) {
  return Catch::Detail::Approx(x).epsilon(1e-14);
}
