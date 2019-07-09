// catch2 header include with
// - additional ostream operator << overloads for some Qt types
// - dbl_approx() convenience function for Approx(x).epsilon(1e-14)

#pragma once

// ostream overloads need to be included before catch header
#include "ostream_overloads.hpp"

#include "ext/catch/catch.hpp"

// add custom Approximation convenience function for doubles
inline Catch::Detail::Approx dbl_approx(double x) {
  return Catch::Detail::Approx(x).epsilon(1e-14);
}
