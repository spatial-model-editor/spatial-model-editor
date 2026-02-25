// DUNE-copasi ini file generation
//  - iniFile class: simple ini file generation one line at a time

#pragma once

#include <QString>

namespace sme {

namespace simulate {

/**
 * @brief Simple helper for building INI text.
 */
class IniFile {
private:
  QString text;

public:
  /**
   * @brief Get generated INI text.
   */
  [[nodiscard]] const QString &getText() const;
  /**
   * @brief Add one-level section.
   */
  void addSection(const QString &str);
  /**
   * @brief Add two-level section.
   */
  void addSection(const QString &str1, const QString &str2);
  /**
   * @brief Add three-level section.
   */
  void addSection(const QString &str1, const QString &str2,
                  const QString &str3);
  /**
   * @brief Add four-level section.
   */
  void addSection(const QString &str1, const QString &str2, const QString &str3,
                  const QString &str4);
  /**
   * @brief Add five-level section.
   */
  void addSection(const QString &str1, const QString &str2, const QString &str3,
                  const QString &str4, const QString &str5);
  /**
   * @brief Add six-level section.
   */
  void addSection(const QString &str1, const QString &str2, const QString &str3,
                  const QString &str4, const QString &str5,
                  const QString &str6);
  /**
   * @brief Add key/value pair (string value).
   */
  void addValue(const QString &var, const QString &value);
  /**
   * @brief Add key/value pair (integer value).
   */
  void addValue(const QString &var, int value);
  /**
   * @brief Add key/value pair (floating value).
   */
  void addValue(const QString &var, double value, int precision);
  /**
   * @brief Clear generated text.
   */
  void clear();
};

} // namespace simulate

} // namespace sme
