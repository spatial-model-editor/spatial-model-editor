// ostream << overloads to log data types not natively supported by spdlog
//  - Qt types
//     - QString
//     - QPoint
//     - QPointF
//     - QSize
//     - QColor
//  - std types
//     - vector<T>
//     - pair<T,U>

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <QByteArray>
#include <QColor>
#include <QLatin1String>
#include <QPoint>
#include <QSize>
#include <QString>

// Qt types

std::ostream &operator<<(std::ostream &os, const QByteArray &value);

std::ostream &operator<<(std::ostream &os, const QLatin1String &value);

std::ostream &operator<<(std::ostream &os, const QString &value);

std::ostream &operator<<(std::ostream &os, const QPoint &value);

std::ostream &operator<<(std::ostream &os, const QPointF &value);

std::ostream &operator<<(std::ostream &os, const QSize &value);

std::ostream &operator<<(std::ostream &os, const QColor &value);

// std types

template <typename T, typename U>
inline std::ostream &operator<<(std::ostream &os, const std::pair<T, U> &p) {
  return os << p.first << ": " << p.second;
}

template <class T>
inline std::ostream &operator<<(std::ostream &os, const std::vector<T> &vec) {
  os << "{ ";
  for (const auto &v : vec) {
    os << v << " ";
  }
  return os << "}";
}
