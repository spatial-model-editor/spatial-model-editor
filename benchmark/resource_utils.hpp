#pragma once

#include <QFile>
#include <QString>
#include <stdexcept>
#include <string>

namespace sme::benchmarking {

inline std::string readResourceTextFile(const QString &filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    throw std::runtime_error(QString("Failed to open resource '%1': %2")
                                 .arg(filename, file.errorString())
                                 .toStdString());
  }
  return file.readAll().toStdString();
}

} // namespace sme::benchmarking
