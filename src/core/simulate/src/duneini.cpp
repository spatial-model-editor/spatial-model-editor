#include "duneini.hpp"
#include "utils.hpp"

namespace sme::simulate {

const QString &IniFile::getText() const { return text; }

void IniFile::addSection(const QString &str) {
  if (!text.isEmpty()) {
    text.append("\n");
  }
  text.append(QString("[%1]\n").arg(str));
}

void IniFile::addSection(const QString &str1, const QString &str2) {
  addSection(QString("%1.%2").arg(str1, str2));
}

void IniFile::addSection(const QString &str1, const QString &str2,
                         const QString &str3) {
  addSection(QString("%1.%2.%3").arg(str1, str2, str3));
}

void IniFile::addSection(const QString &str1, const QString &str2,
                         const QString &str3, const QString &str4) {
  addSection(QString("%1.%2.%3.%4").arg(str1, str2, str3, str4));
}

void IniFile::addSection(const QString &str1, const QString &str2,
                         const QString &str3, const QString &str4,
                         const QString &str5) {
  addSection(QString("%1.%2.%3.%4.%5").arg(str1, str2, str3, str4, str5));
}

void IniFile::addSection(const QString &str1, const QString &str2,
                         const QString &str3, const QString &str4,
                         const QString &str5, const QString &str6) {
  addSection(
      QString("%1.%2.%3.%4.%5.%6").arg(str1, str2, str3, str4, str5, str6));
}

void IniFile::addValue(const QString &var, const QString &value) {
  text.append(QString("%1 = %2\n").arg(var, value));
}

void IniFile::addValue(const QString &var, int value) {
  addValue(var, QString::number(value));
}

void IniFile::addValue(const QString &var, double value, int precision) {
  addValue(var, common::dblToQStr(value, precision));
}

void IniFile::clear() { text.clear(); }

} // namespace sme::simulate
