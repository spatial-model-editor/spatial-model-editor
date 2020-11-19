// DUNE-copasi ini file generation
//  - iniFile class: simple ini file generation one line at a time

#pragma once

#include <QString>

namespace simulate {

class IniFile {
private:
  QString text;

public:
  const QString &getText() const;
  void addSection(const QString &str);
  void addSection(const QString &str1, const QString &str2);
  void addSection(const QString &str1, const QString &str2,
                  const QString &str3);
  void addSection(const QString &str1, const QString &str2, const QString &str3,
                  const QString &str4);
  void addSection(const QString &str1, const QString &str2, const QString &str3,
                  const QString &str4, const QString &str5);
  void addSection(const QString &str1, const QString &str2, const QString &str3,
                  const QString &str4, const QString &str5,
                  const QString &str6);
  void addValue(const QString &var, const QString &value);
  void addValue(const QString &var, int value);
  void addValue(const QString &var, double value, int precision);
  void clear();
};

} // namespace simulate
