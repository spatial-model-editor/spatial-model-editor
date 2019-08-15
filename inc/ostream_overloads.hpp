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

inline std::ostream &operator<<(std::ostream &os, const QPointF &value) {
  return os << "(" << value.x() << "," << value.y() << ")";
}

inline std::ostream &operator<<(std::ostream &os, const QSize &value) {
  return os << "(" << value.width() << "," << value.height() << ")";
}

inline std::ostream &operator<<(std::ostream &os, const QColor &value) {
  return os << value.rgba();
}

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
