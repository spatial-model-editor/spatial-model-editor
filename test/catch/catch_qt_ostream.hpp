// ostream operator << overloads to enable catch2 to display some Qt types

#pragma once

#include <iostream>

#include <QByteArray>
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
