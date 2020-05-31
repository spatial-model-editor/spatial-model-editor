// catch2 header include with
//   - dbl_approx() convenience function for Approx(x).epsilon(1e-14)
//   - ostream overloads for some Qt types

#pragma once
#include <QPoint>
#include <iosfwd>
#include <utility>

class QString;
class QPointF;
class QSize;
class QSizeF;

std::ostream& operator<<(std::ostream& os, QString const& value);
std::ostream& operator<<(std::ostream& os, QPoint const& value);
std::ostream& operator<<(std::ostream& os, QPointF const& value);
std::ostream& operator<<(std::ostream& os, QSize const& value);
std::ostream& operator<<(std::ostream& os, QSizeF const& value);
std::ostream& operator<<(std::ostream& os,
                         std::pair<QPoint, QPoint> const& value);

#define CATCH_CONFIG_FAST_COMPILE
#define CATCH_CONFIG_DISABLE_MATCHERS

#include <catch2/catch.hpp>

// add custom Approximation convenience function for doubles
inline Catch::Detail::Approx dbl_approx(double x) {
  return Catch::Detail::Approx(x).epsilon(1e-14);
}
