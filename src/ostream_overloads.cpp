#include "ostream_overloads.hpp"

std::ostream &operator<<(std::ostream &os, const QByteArray &value) {
  return os << '"' << (value.isEmpty() ? "" : value.constData()) << '"';
}

std::ostream &operator<<(std::ostream &os, const QLatin1String &value) {
  return os << '"' << value.latin1() << '"';
}

std::ostream &operator<<(std::ostream &os, const QString &value) {
  return os << value.toLocal8Bit();
}

std::ostream &operator<<(std::ostream &os, const QPoint &value) {
  return os << "(" << value.x() << "," << value.y() << ")";
}

std::ostream &operator<<(std::ostream &os, const QPointF &value) {
  return os << "(" << value.x() << "," << value.y() << ")";
}

std::ostream &operator<<(std::ostream &os, const QSize &value) {
  return os << value.width() << "x" << value.height();
}

std::ostream &operator<<(std::ostream &os, const QSizeF &value) {
  return os << value.width() << "x" << value.height();
}

std::ostream &operator<<(std::ostream &os, const QColor &value) {
  return os << std::hex << value.rgba();
}
