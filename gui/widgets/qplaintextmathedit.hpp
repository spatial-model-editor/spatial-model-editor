// QPlainTextMathEdit
//  - a modified QPlainTextEdit for editing sbml math
//  - syntax highlighting
//  - syntax checking
//  - autocomplete
//  - evaluation
//  - emits a signal when text changed, along with bool isValid, and
//  errormessage

#pragma once

#include "sme/model_parameters.hpp"
#include "sme/symbolic.hpp"
#include <QCompleter>
#include <QPlainTextEdit>
#include <QStringListModel>

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
  void reset();
  void addFunction(const sme::common::SymbolicFunction &function);
  void removeFunction(const std::string &functionId);
  void setConstants(const std::vector<sme::model::IdNameValue> &constants = {});
  void updateCompleter();

signals:
  void mathChanged(const QString &math, bool valid,
                   const QString &errorMessage);

private:
  using StringStringMap = std::map<std::string, std::string, std::less<>>;
  QCompleter completer;
  QStringListModel stringListModel;
  sme::common::Symbolic sym;
  std::vector<double> result{0.0};
  const QColor colourValid{QColor(200, 255, 200)};
  const QColor colourInvalid{QColor(255, 150, 150)};
  std::vector<std::string> vars;
  std::vector<sme::common::SymbolicFunction> functions;
  std::vector<std::pair<std::string, double>> consts{};
  StringStringMap mapVarsToDisplayNames;
  StringStringMap mapDisplayNamesToVars;
  StringStringMap mapFuncsToDisplayNames;
  StringStringMap mapDisplayNamesToFuncs;
  QString currentDisplayMath;
  std::string currentVariableMath;
  QString currentErrorMessage;
  bool expressionIsValid{false};
  qsizetype currentWordStartPos{0};
  qsizetype currentWordEndPos{0};
  void clearFunctions();
  std::pair<std::string, QString>
  displayNamesToVariables(const std::string &expr) const;
  std::string variablesToDisplayNames(const std::string &expr) const;
  void qPlainTextEdit_textChanged();
  void qPlainTextEdit_cursorPositionChanged();
  QString getCurrentWord();
  void insertCompletion(const QString &completion);
  void keyPressEvent(QKeyEvent *e) override;
};
