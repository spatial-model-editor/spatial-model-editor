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

std::ostream &operator<<(std::ostream &os, QString const &value);
std::ostream &operator<<(std::ostream &os, QPoint const &value);
std::ostream &operator<<(std::ostream &os, QPointF const &value);
std::ostream &operator<<(std::ostream &os, QSize const &value);
std::ostream &operator<<(std::ostream &os, QSizeF const &value);
std::ostream &operator<<(std::ostream &os,
                         std::pair<QPoint, QPoint> const &value);

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

// add custom Approximation convenience function for doubles
inline Catch::Approx dbl_approx(double x) {
  return Catch::Approx(x).epsilon(1e-14);
}
