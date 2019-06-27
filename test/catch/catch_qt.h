// catch2 header include with
// - additional ostream operator << overloads for some Qt types
// - dbl_approx() convenience function for Approx(x).epsilon(1e-14)

#pragma once

#include <iostream>
#include <utility>

#include <QByteArray>
#include <QColor>
#include <QLatin1String>
#include <QPoint>
#include <QSize>
#include <QString>

inline std::ostream &operator<<(std::ostream &os, const QByteArray &value) {
  return os << '"' << (value.isEmpty() ? "" : value.constData()) << '"';
}

inline std::ostream &operator<<(std::ostream &os, const QLatin1String &value) {
  return os << '"' << value.latin1() << '"';
}

inline std::ostream &operator<<(std::ostream &os, const QString &value) {
  return os << value.toLocal8Bit();
}

inline std::ostream &operator<<(std::ostream &os, const QPoint &value) {
  return os << "(" << value.x() << "," << value.y() << ")";
}

inline std::ostream &operator<<(std::ostream &os, const QSize &value) {
  return os << "(" << value.width() << "," << value.height() << ")";
}

inline std::ostream &operator<<(std::ostream &os, const QColor &value) {
  return os << value.rgba();
}

template <typename T>
inline std::ostream &operator<<(std::ostream &os, const std::pair<T, T> &p) {
  return os << "{ " << p.first << ", " << p.second << " }";
}

#include "catch.hpp"

// add custom Approximation convenience function for doubles
inline Catch::Detail::Approx dbl_approx(double x) {
  return Catch::Detail::Approx(x).epsilon(1e-14);
}
