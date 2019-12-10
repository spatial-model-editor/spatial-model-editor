// QPlainTextMathEdit
//  - a modified QPlainTextEdit for editing math
//  - syntax highlighting
//  - syntax checking
//  - emits a signal when text changed, along with bool isValid, and
//  errormessage

#pragma once

#include <QPlainTextEdit>

#include "symbolic.hpp"

class QPlainTextMathEdit : public QPlainTextEdit {
  Q_OBJECT
 public:
  explicit QPlainTextMathEdit(QWidget *parent = nullptr);
  bool mathIsValid() const;
  const QString &getMath() const;
  const std::string &getVariableMath() const;
  void importVariableMath(const std::string &expr);
  void compileMath();
  double evaluateMath(const std::vector<double> &variables = {});
  const QString &getErrorMessage() const;
  const std::vector<std::string> &getVariables() const;
  void clearVariables();
  void setVariables(const std::vector<std::string> &variables);
  void addVariable(const std::string &variable,
                   const std::string &displayName = {});
  void removeVariable(const std::string &variable);

 signals:
  void mathChanged(const QString &math, bool valid,
                   const QString &errorMessage);

 private:
  symbolic::Symbolic sym;
  std::vector<double> result{0.0};
  const QColor colourValid = QColor(200, 255, 200);
  const QColor colourInvalid = QColor(255, 150, 150);
  const char *illegalChars = "%@&!";
  std::vector<std::string> vars;
  std::map<std::string, std::string> mapVarsToDisplayNames;
  std::map<std::string, std::string> mapDisplayNamesToVars;
  QString currentDisplayMath;
  std::string currentVariableMath;
  QString currentErrorMessage;
  bool expressionIsValid = false;
  std::pair<std::string, QString> displayNamesToVariables(
      const std::string &expr) const;
  std::string variablesToDisplayNames(const std::string &expr) const;
  void qPlainTextEdit_textChanged();
  void qPlainTextEdit_cursorPositionChanged();
};
