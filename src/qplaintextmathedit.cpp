#include "qplaintextmathedit.hpp"

#include <symengine/symengine_exception.h>

#include <algorithm>

#include "logger.hpp"

bool QPlainTextMathEdit::mathIsValid() const { return expressionIsValid; }

const QString& QPlainTextMathEdit::getMath() const { return currentMath; }

void QPlainTextMathEdit::compileMath() { sym.compile(); }

double QPlainTextMathEdit::evaluateMath(const std::vector<double>& variables) {
  sym.evalLLVM(result, variables);
  return result[0];
}

const QString& QPlainTextMathEdit::getErrorMessage() const {
  return currentErrorMessage;
}

const std::vector<std::string>& QPlainTextMathEdit::getVariables() const {
  return vars;
}

void QPlainTextMathEdit::setVariables(
    const std::vector<std::string>& variables) {
  vars = variables;
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::addVariable(const std::string& variable) {
  vars.push_back(variable);
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::removeVariable(const std::string& variable) {
  vars.erase(std::remove(vars.begin(), vars.end(), variable), vars.end());
  qPlainTextEdit_textChanged();
}

QPlainTextMathEdit::QPlainTextMathEdit(QWidget* parent)
    : QPlainTextEdit(parent) {
  connect(this, &QPlainTextEdit::textChanged, this,
          &QPlainTextMathEdit::qPlainTextEdit_textChanged);
  connect(this, &QPlainTextEdit::cursorPositionChanged, this,
          &QPlainTextMathEdit::qPlainTextEdit_cursorPositionChanged);
}

void QPlainTextMathEdit::qPlainTextEdit_textChanged() {
  expressionIsValid = false;
  currentMath.clear();
  // check for illegal chars
  std::string newExpr = toPlainText().toStdString();
  if (auto i = newExpr.find_first_of(illegalChars); i != std::string::npos) {
    currentErrorMessage = QString("Illegal character: %1").arg(newExpr[i]);
  } else {
    try {
      // parse (but don't compile) symbolic expression
      sym = symbolic::Symbolic(newExpr, vars, {}, false);
      expressionIsValid = true;
      currentErrorMessage = "";
      currentMath = sym.simplify().c_str();
    } catch (SymEngine::SymEngineException& e) {
      // if SymEngine failed to parse, capture error message
      currentErrorMessage = e.what();
    }
  }
  emit mathChanged(currentMath, expressionIsValid, currentErrorMessage);
}

static std::pair<int, bool> getClosingBracket(const QString& expr, int pos,
                                              int sign) {
  int len = 0;
  int count = sign;
  int iEnd = (sign < 0) ? -1 : expr.size();
  for (int i = pos + sign; i != iEnd; i += sign) {
    ++len;
    if (expr[i] == ')') {
      --count;
    } else if (expr[i] == '(') {
      ++count;
    }
    if (count == 0) {
      // found matching bracket
      return {len + 1, true};
    }
  }
  // did not find closing bracket
  return {len + 1, false};
}

void QPlainTextMathEdit::qPlainTextEdit_cursorPositionChanged() {
  // very basic syntax highlighting:
  // if cursor is before a '(' or after a ')'
  // highlight text between matching braces in green
  // if there is no matching brace, highlight with red
  auto expr = toPlainText();
  QTextEdit::ExtraSelection s{textCursor(), {}};
  int i = s.cursor.position();
  if (expr[i] == '(') {
    auto [len, valid] = getClosingBracket(expr, i, +1);
    const auto& col = valid ? colourValid : colourInvalid;
    s.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                          len);
    s.format.setBackground(QBrush(col));
  } else if (i > 0 && expr[i - 1] == ')') {
    auto [len, valid] = getClosingBracket(expr, i - 1, -1);
    const auto& col = valid ? colourValid : colourInvalid;
    s.cursor.movePosition(QTextCursor::PreviousCharacter,
                          QTextCursor::KeepAnchor, len);
    s.format.setBackground(QBrush(col));
  }
  setExtraSelections({s});
}
