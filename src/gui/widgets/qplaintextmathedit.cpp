#include "qplaintextmathedit.hpp"

#include <symengine/symengine_exception.h>

#include <QString>
#include <algorithm>
#include <locale>

#include "logger.hpp"

bool QPlainTextMathEdit::mathIsValid() const { return expressionIsValid; }

const QString& QPlainTextMathEdit::getMath() const {
  return currentDisplayMath;
}

const std::string& QPlainTextMathEdit::getVariableMath() const {
  return currentVariableMath;
}

void QPlainTextMathEdit::importVariableMath(const std::string& expr) {
  setPlainText(variablesToDisplayNames(expr).c_str());
}

void QPlainTextMathEdit::compileMath() { sym.compile(); }

double QPlainTextMathEdit::evaluateMath(const std::vector<double>& variables) {
  sym.eval(result, variables);
  return result[0];
}

const QString& QPlainTextMathEdit::getErrorMessage() const {
  return currentErrorMessage;
}

const std::vector<std::string>& QPlainTextMathEdit::getVariables() const {
  return vars;
}

void QPlainTextMathEdit::clearVariables() {
  vars.clear();
  mapDisplayNamesToVars.clear();
  mapVarsToDisplayNames.clear();
}

void QPlainTextMathEdit::setVariables(
    const std::vector<std::string>& variables) {
  clearVariables();
  for (const auto& v : variables) {
    addVariable(v, v);
  }
  qPlainTextEdit_textChanged();
}

static bool isValidSymbol(const std::string& name) {
  // first char must be a letter or underscore
  if (auto c = name.front();
      !(std::isalpha(c, std::locale::classic()) || c == '_')) {
    return false;
  }
  // other chars must be letters, numbers or underscores
  for (auto c : name) {
    if (!(std::isalnum(c, std::locale::classic()) || c == '_')) {
      return false;
    }
  }
  return true;
}

void QPlainTextMathEdit::addVariable(const std::string& variable,
                                     const std::string& displayName) {
  vars.push_back(variable);
  auto name = displayName;
  if (displayName.empty()) {
    name = variable;
  }
  mapDisplayNamesToVars[name] = variable;
  if (isValidSymbol(name)) {
    mapVarsToDisplayNames[variable] = name;
  } else {
    mapVarsToDisplayNames[variable] = "\"" + name + "\"";
  }
  qPlainTextEdit_textChanged();
}

void QPlainTextMathEdit::removeVariable(const std::string& variable) {
  vars.erase(std::remove(vars.begin(), vars.end(), variable), vars.end());
  if (auto iter = mapVarsToDisplayNames.find(variable);
      iter != mapVarsToDisplayNames.cend()) {
    SPDLOG_TRACE("removing var: {}", variable);
    auto displayName = iter->second;
    std::size_t nQuoteChars = 0;
    // remove quotes if present
    if (displayName.front() == '"' && displayName.back() == '"') {
      nQuoteChars = 1;
    }
    SPDLOG_TRACE("  -> display name {}", displayName);
    mapDisplayNamesToVars.erase(
        displayName.substr(nQuoteChars, displayName.size() - 2 * nQuoteChars));
    mapVarsToDisplayNames.erase(iter);
  }
  qPlainTextEdit_textChanged();
}

QPlainTextMathEdit::QPlainTextMathEdit(QWidget* parent)
    : QPlainTextEdit(parent) {
  connect(this, &QPlainTextEdit::textChanged, this,
          &QPlainTextMathEdit::qPlainTextEdit_textChanged);
  connect(this, &QPlainTextEdit::cursorPositionChanged, this,
          &QPlainTextMathEdit::qPlainTextEdit_cursorPositionChanged);
}

// - iterate through each variable in expr
// - a variable is any text with a delimeter char before and after it
// - look-up variable in map and replace it with result
// - if map contents are not a valid symbol, wrap it in quotes
// - if variable not found in map, return instead a QString error message
static std::pair<std::string, QString> substitute(
    const std::string& expr, const std::map<std::string, std::string>& map,
    const std::string& delimeters = "()-^*/+, ") {
  SPDLOG_DEBUG("expr: {}", expr);
  std::string out;
  out.reserve(expr.size());
  // skip over any initial delimeters
  auto start = expr.find_first_not_of(delimeters);
  // append them to output
  out.append(expr.substr(0, start));
  while (start != std::string::npos) {
    // find next delimeter
    auto end = expr.find_first_of(delimeters, start);
    // extract variable
    std::string var = expr.substr(start, end - start);
    SPDLOG_DEBUG("  - var {}", var);
    if (auto iter = map.find(var); iter != map.cend()) {
      // replace variable with map result
      SPDLOG_DEBUG("    -> {} ", iter->second);
      out.append(iter->second);
    } else {
      // if not found, check if it is a number, if so append it and continue
      bool ok;
      QString(var.c_str()).toDouble(&ok);
      if (ok) {
        out.append(var);
      } else {
        // not found and not a number: return error message
        SPDLOG_DEBUG("    -> not found");
        return {out, QString("name '%1' not found").arg(var.c_str())};
      }
    }
    if (end == std::string::npos) {
      break;
    }
    // skip over any delimeters to start of next variable
    start = expr.find_first_not_of(delimeters, end);
    // append them to output
    out.append(expr.substr(end, start - end));
  }
  SPDLOG_DEBUG("  -> {}", out);
  return {out, {}};
}

std::string QPlainTextMathEdit::variablesToDisplayNames(
    const std::string& expr) const {
  const auto& map = mapVarsToDisplayNames;
  if (map.empty()) {
    return expr;
  }
  return substitute(expr, map).first;
}

std::pair<std::string, QString> QPlainTextMathEdit::displayNamesToVariables(
    const std::string& expr) const {
  std::string out;
  const char quoteChar = '"';
  const auto& map = mapDisplayNamesToVars;
  if (map.empty()) {
    return {expr, ""};
  }
  // find first quote char
  auto start = expr.find(quoteChar);
  if (start > 0) {
    // substitute any text before opening quote
    auto subs = substitute(expr.substr(0, start), map);
    if (subs.second.isEmpty()) {
      out.append(subs.first);
    } else {
      return subs;
    }
  }
  while (start != std::string::npos) {
    // skip over opening quote char
    ++start;
    // find closing quote char
    auto end = expr.find(quoteChar, start);
    if (end == std::string::npos) {
      SPDLOG_DEBUG("  - no closing quote");
      return {out, "no closing quote"};
    }
    // extract name & replace with variable if found
    std::string name = expr.substr(start, end - start);
    SPDLOG_TRACE("  - name {} (chars {}-{})", name, start, end);
    if (auto iter = map.find(name); iter != map.cend()) {
      out.append(iter->second);
    } else {
      SPDLOG_DEBUG("  - name {} not found", name);
      return {out, QString("name '%1' not found").arg(name.c_str())};
    }
    SPDLOG_TRACE("  - out: {}", out);
    // skip over closing quote char
    ++end;
    // find next opening quote char
    start = expr.find(quoteChar, end);
    if (start > end) {
      // substitute any text before next "
      auto subs = substitute(expr.substr(end, start - end), map);
      if (subs.second.isEmpty()) {
        out.append(subs.first);
      } else {
        return subs;
      }
    }
  }
  SPDLOG_DEBUG(" -> {}", out);
  return {out, {}};
}

void QPlainTextMathEdit::qPlainTextEdit_textChanged() {
  expressionIsValid = false;
  currentDisplayMath.clear();
  currentVariableMath.clear();
  currentErrorMessage.clear();
  // convert display names in expression to variables
  auto [newExpr, errorMessage] =
      displayNamesToVariables(toPlainText().toStdString());
  if (!errorMessage.isEmpty()) {
    currentErrorMessage = errorMessage;
  }
  if (auto i = newExpr.find_first_of(illegalChars); i != std::string::npos) {
    // check for illegal chars
    currentErrorMessage = QString("Illegal character: %1").arg(newExpr[i]);
  }
  if (currentErrorMessage.isEmpty()) {
    try {
      // parse (but don't compile) symbolic expression
      SPDLOG_DEBUG("parsing {}", newExpr);
      sym = symbolic::Symbolic(newExpr, vars, {}, false);
      expressionIsValid = true;
      currentErrorMessage = "";
      currentVariableMath = sym.simplify();
      currentDisplayMath = variablesToDisplayNames(currentVariableMath).c_str();
    } catch (const SymEngine::SymEngineException& e) {
      // if SymEngine failed to parse, capture error message
      currentErrorMessage = e.what();
    }
  }
  emit mathChanged(currentDisplayMath, expressionIsValid, currentErrorMessage);
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
