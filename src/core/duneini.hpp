// DUNE-copasi ini file generation
//  - DuneConverter: construct DUNE ini file
//  - iniFile class: simple ini file generation one line at a time

#pragma once

#include <QString>
#include <map>
#include <memory>
#include <unordered_set>

namespace sbml {
class SbmlDocWrapper;
}

namespace dune {

class IniFile {
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
  void addValue(const QString &var, double value, int precision);
  void clear();
};

class DuneConverter {
 public:
  explicit DuneConverter(const sbml::SbmlDocWrapper &SbmlDoc, double dt = 1e-2,
                         int doublePrecision = 15);
  QString getIniFile() const;
  const std::unordered_set<int> &getGMSHCompIndices() const;
  bool hasIndependentCompartments() const;

 private:
  const sbml::SbmlDocWrapper &doc;
  IniFile ini;
  std::unordered_set<int> gmshCompIndices;
  bool independentCompartments = true;
};

}  // namespace dune
