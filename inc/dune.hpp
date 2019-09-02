// DUNE ini file converter
//  - constructs DUNE ini file
//  - uses iniFile class to construct simple ini file one line at a time

#pragma once

#include "sbml.hpp"

namespace dune {

class iniFile {
 private:
  QString text;

 public:
  const QString &getText() const;
  void addSection(const QString &str);
  void addSection(const QString &str1, const QString &str2);
  void addSection(const QString &str1, const QString &str2,
                  const QString &str3);
  void addValue(const QString &var, const QString &value);
  void addValue(const QString &var, int value);
  void addValue(const QString &var, double value, int precision = 17);
  void clear();
};

class DuneConverter {
 public:
  explicit DuneConverter(const sbml::SbmlDocWrapper &SbmlDoc,
                         int doublePrecision = 17);
  QString getIniFile() const;

 private:
  const sbml::SbmlDocWrapper &doc;
  iniFile ini;
};

}  // namespace dune
