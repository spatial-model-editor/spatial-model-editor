// operator << overloads to enable catch2 to display some Qt types

#ifndef QT_OSTREAM_H
#define QT_OSTREAM_H

#include <QByteArray>
#include <QLatin1String>
#include <QString>
#include <iostream>

inline std::ostream &operator<<(std::ostream &os, const QByteArray &value) {
  return os << '"' << (value.isEmpty() ? "" : value.constData()) << '"';
}

inline std::ostream &operator<<(std::ostream &os, const QLatin1String &value) {
  return os << '"' << value.latin1() << '"';
}

inline std::ostream &operator<<(std::ostream &os, const QString &value) {
  return os << value.toLocal8Bit();
}

#endif  // QT_OSTREAM_H
