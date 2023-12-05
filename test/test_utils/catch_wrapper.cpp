#include "catch_wrapper.hpp"

#include "sme/voxel.hpp"
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <ostream>

std::ostream &operator<<(std::ostream &os, QString const &value) {
  os << '"' << value.toStdString() << '"';
  return os;
}
std::ostream &operator<<(std::ostream &os, QPoint const &value) {
  os << '(' << value.x() << ',' << value.y() << ')';
  return os;
}
std::ostream &operator<<(std::ostream &os, QPointF const &value) {
  os << '(' << value.x() << ',' << value.y() << ')';
  return os;
}
std::ostream &operator<<(std::ostream &os, QSize const &value) {
  os << '(' << value.width() << ',' << value.height() << ')';
  return os;
}
std::ostream &operator<<(std::ostream &os, QSizeF const &value) {
  os << '(' << value.width() << ',' << value.height() << ')';
  return os;
}
std::ostream &operator<<(std::ostream &os,
                         std::pair<QPoint, QPoint> const &value) {
  os << '{' << value.first << ',' << value.second << '}';
  return os;
}
std::ostream &operator<<(std::ostream &os, sme::common::Volume const &value) {
  os << '{' << value.width() << " x " << value.height() << " x "
     << value.depth() << '}';
  return os;
}
